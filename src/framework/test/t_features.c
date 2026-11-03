// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "test.h"
#include "t_features.h"

bool
cru_feature_has_extension(const struct cru_feature_context *context,
                          const char *name)
{
    const VkDeviceCreateInfo *info = context->device_info;
    for (uint32_t i = 0; i < info->enabledExtensionCount; i++) {
        if (strcmp(name, info->ppEnabledExtensionNames[i]) == 0)
            return true;
    }
    for (uint32_t i = 0; i < context->instance_extension_count; i++) {
        if (strcmp(name, context->instance_extensions[i].extensionName) == 0)
            return true;
    }
    return false;
}

static void
exclude_extension(VkDeviceCreateInfo *info, const char *name)
{
    for (uint32_t i = 0; i < info->enabledExtensionCount; i++) {
        if (strcmp(name, info->ppEnabledExtensionNames[i]) != 0)
            continue;

        const char **extensions = malloc(info->enabledExtensionCount * sizeof(*extensions));
        t_assert(extensions != NULL);
        t_cleanup_push_free(extensions);
        uint32_t count = 0;
        for (uint32_t j = 0; j < info->enabledExtensionCount; j++) {
            if (strcmp(name, info->ppEnabledExtensionNames[j]) != 0)
                extensions[count++] = info->ppEnabledExtensionNames[j];
        }
        info->ppEnabledExtensionNames = extensions;
        info->enabledExtensionCount = count;
        return;
    }
}

static void
append(void *chain, void *node)
{
    VkBaseOutStructure *tail = chain;
    while (tail->pNext)
        tail = tail->pNext;
    tail->pNext = node;
}

void *
cru_find_feature_struct(const void *chain, VkStructureType type)
{
    for (const VkBaseInStructure *node = chain; node; node = node->pNext) {
        if (node->sType == type)
            return (void *)node;
    }
    return NULL;
}

static void *
new_structure(VkStructureType type, size_t size)
{
    VkBaseOutStructure *node = calloc(1, size);
    t_assert(node != NULL);
    t_cleanup_push_free(node);
    node->sType = type;
    return node;
}

struct cru_feature_state
cru_require_feature_struct(struct cru_feature_setup *setup,
                            VkStructureType type, size_t size)
{
    if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)
        return (struct cru_feature_state) { &setup->supported, &setup->enabled };

    struct cru_feature_state state = {
        .supported = cru_find_feature_struct(&setup->supported, type),
        .enabled = cru_find_feature_struct(setup->context->device_info->pNext, type),
    };
    if (!state.supported) {
        state.supported = new_structure(type, size);
        append(&setup->supported, state.supported);
    }
    if (!state.enabled) {
        state.enabled = new_structure(type, size);
        append(setup->context->device_info, state.enabled);
    }
    return state;
}

void
cru_add_feature_requirement(struct cru_feature_setup *setup, const char *name,
                             const VkBool32 *supported, VkBool32 *enabled)
{
    if (!setup->requirements) {
        setup->requirements = calloc(cru_feature_count, sizeof(*setup->requirements));
        t_assert(setup->requirements != NULL);
        t_cleanup_push_free(setup->requirements);
    }
    setup->requirements[setup->requirement_count++] = (struct cru_feature_requirement) {
        name, supported, enabled,
    };
}

const char *
t_setup_required_features(VkDeviceCreateInfo *device_info,
                          uint32_t api_version)
{
    GET_CURRENT_TEST(t);
    const struct cru_features *required = &t->def->features;
    // BDA variants have different semantics despite their matching member names.
    // An explicit requirement selects the variant, not driver advertisement order.
    bool core_bda = required->bufferDeviceAddress || required->bufferDeviceAddressCaptureReplay ||
                    required->bufferDeviceAddressMultiDevice;
    bool ext_bda = required->bufferDeviceAddressEXT || required->bufferDeviceAddressCaptureReplayEXT ||
                   required->bufferDeviceAddressMultiDeviceEXT;
    if (core_bda && ext_bda)
        return "bufferDeviceAddress + bufferDeviceAddressEXT";
    if (core_bda)
        exclude_extension(device_info, "VK_EXT_buffer_device_address");
    else if (ext_bda)
        exclude_extension(device_info, "VK_KHR_buffer_device_address");

    const struct cru_feature_context context = {
        .api_version = MIN(api_version, t->vk.physical_dev_props.apiVersion),
        .instance_extension_count = t->vk.instance_extension_count,
        .instance_extensions = t->vk.instance_extension_props,
        .device_info = device_info,
    };
    struct cru_feature_setup setup = {
        .context = &context,
        .supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .enabled = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .features = *device_info->pEnabledFeatures,
        },
    };
    const char *missing = cru_select_features(&setup, required);
    if (missing)
        return missing;
    if (!setup.any) {
        cru_record_enabled_features(&context, &t->vk.enabled_features);
        return NULL;
    }

    vkGetPhysicalDeviceFeatures2(t->vk.physical_dev, &setup.supported);

    // Some promoted features were enabled solely by enabling an extension.
    // Preserve that enablement when adding an aggregate core feature struct.
    cru_preserve_implicit_features(&setup);
    for (size_t i = 0; i < setup.requirement_count; i++) {
        const struct cru_feature_requirement *req = &setup.requirements[i];
        if (!*req->supported)
            return req->name;
        *req->enabled = VK_TRUE;
    }
    *(VkPhysicalDeviceFeatures *)device_info->pEnabledFeatures = setup.enabled.features;
    cru_record_enabled_features(&context, &t->vk.enabled_features);
    return NULL;
}
