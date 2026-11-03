// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>

#include "t_features_gen.h"
#include "util/macros.h"
#include "util/vk_wrapper.h"

struct cru_feature_context {
    uint32_t api_version;
    uint32_t instance_extension_count;
    const VkExtensionProperties *instance_extensions;
    VkDeviceCreateInfo *device_info;
};

struct cru_feature_state {
    void *supported;
    void *enabled;
};

struct cru_feature_requirement {
    const char *name;
    const VkBool32 *supported;
    VkBool32 *enabled;
};

struct cru_feature_setup {
    const struct cru_feature_context *context;
    VkPhysicalDeviceFeatures2 supported;
    VkPhysicalDeviceFeatures2 enabled;
    struct cru_feature_requirement *requirements;
    size_t requirement_count;
    bool any;
};

bool cru_feature_has_extension(const struct cru_feature_context *context,
                                const char *name);
void *cru_find_feature_struct(const void *chain, VkStructureType type);
struct cru_feature_state cru_require_feature_struct(struct cru_feature_setup *setup,
                                                     VkStructureType type, size_t size);
void cru_add_feature_requirement(struct cru_feature_setup *setup, const char *name,
                                 const VkBool32 *supported, VkBool32 *enabled);

// Generated direct accesses to the flat feature set and Vulkan members.
extern const size_t cru_feature_count;
const char *cru_select_features(struct cru_feature_setup *setup,
                                 const struct cru_features *required);
void cru_preserve_implicit_features(const struct cru_feature_setup *setup);
void cru_record_enabled_features(const struct cru_feature_context *context,
                                  struct cru_features *features);

// Return an unsatisfied feature requirement, or NULL on success.
const char *t_setup_required_features(VkDeviceCreateInfo *device_info,
                                      uint32_t api_version);
