// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

// \file
// Reproduce the Intel Anv regression from f33b98ff.

/*
 * tl;dr: Blorp indirect memory copies between buffers use ugm stores, which use
 * the dataport cache. Commit f33b98ff18df ("anv: limit flushing for transfer
 * writes on RCS on Xe2+") changed that behavior, causing a regression when a
 * buffer is copied indirectly and then read from the sampler. This test should
 * exercise that behavior.
 *
 * The test creates 3 buffers: 'a', 'b' and 'c'. It initially maps the buffers
 * and fills them with 0xAA, 0xBB and 0xCC, respectively.
 *
 * It copies data from 'a' to 'b' using an indirect buffer-to-buffer copy.
 * It copies data from 'b' to 'img' using a normal buffer-to-image copy.
 * It copies data from 'img' to 'c' using a normal image-to-buffer copy.
 * It maps 'c' again and reads from it.
 *
 * The expectation is, of course, that the contents of 'a' traveled all the way
 * to 'c'. If any of the copies or barriers didn't work, the results should
 * reflect that.
 *
 * Originally, Anv had a flushing bug where the data written to 'b' got stuck in
 * an unflushed data cache, so when the 'b -> img' happened, it used the
 * sampler, which ignored the contents of the unflushed data cache, copying 0xBB
 * to 'img' instead of the expected 0xAA.
 *
 * The test could also fail if any of the copies simply don't work.
 */

#include "tapi/t.h"

struct test_params {
    uint32_t img_w;
    uint32_t img_h;
    bool use_tiling;
};

static const VkFormat format = VK_FORMAT_R8G8B8A8_UINT;
static const uint32_t bpp = 4;

struct test_resources {
    uint64_t buf_size;

    VkBuffer buf_a;
    VkBuffer buf_b;
    VkBuffer buf_c;
    VkBuffer buf_indirect;

    VkImage img;

    VkDeviceMemory mem_a;
    VkDeviceMemory mem_b;
    VkDeviceMemory mem_c;
    VkDeviceMemory mem_indirect;
    VkDeviceMemory mem_img;

    VkDeviceAddress buf_a_addr;
    VkDeviceAddress buf_b_addr;
    VkDeviceAddress buf_indirect_addr;

    uint8_t *ptr_a;
    uint8_t *ptr_b;
    uint8_t *ptr_c;
    VkCopyMemoryIndirectCommandKHR *ptr_indirect;
};

static PFN_vkCmdCopyMemoryIndirectKHR copy_mem_indirect_fn;

static void
check_test_requirements(void)
{
    t_require_ext("VK_KHR_copy_memory_indirect");

    VkPhysicalDeviceCopyMemoryIndirectPropertiesKHR copy_props = {};
    copy_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &copy_props;

    vkGetPhysicalDeviceProperties2(t_physical_dev, &props);

    VkPhysicalDeviceCopyMemoryIndirectFeaturesKHR copy_features = {};
    copy_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_FEATURES_KHR;

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &copy_features;

    vkGetPhysicalDeviceFeatures2(t_physical_dev, &features);

    if (props.properties.apiVersion < VK_API_VERSION_1_3)
        t_skipf("Vulkan version is too old");

    if (!copy_features.indirectMemoryCopy)
        t_skipf("no copy_features.indirectMemoryCopy");

    uint32_t n_fam_props = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(t_physical_dev, &n_fam_props,
                                             NULL);
    VkQueueFamilyProperties *fam_props =
        xmallocn(n_fam_props, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(t_physical_dev, &n_fam_props,
                                             fam_props);
    VkQueueFlags test_queue_flags = fam_props[t_queue_family].queueFlags;

    free(fam_props);

    if (!(copy_props.supportedQueues & test_queue_flags))
        t_skipf("queue family not supported");
}

/* We can't use the qonos wrappers due to 'flags_info'. */
static VkDeviceMemory
alloc_mem_for_buf(VkBuffer buf)
{
    VkDeviceMemory mem;
    VkResult result;

    VkMemoryPropertyFlagBits mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(t_device, buf, &mem_reqs);

    uint32_t mem_index = qoFindMemoryTypeWithProperties(mem_reqs.memoryTypeBits,
                                                        mem_props);

    VkMemoryAllocateFlagsInfo flags_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0,
    };
    VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &flags_info,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = mem_index,
    };
    result = vkAllocateMemory(t_device, &mem_alloc_info, NULL, &mem);
    t_assert(result == VK_SUCCESS);
    t_assert(mem != VK_NULL_HANDLE);
    t_cleanup_push_vk_device_memory(t_device, mem);

    qoBindBufferMemory(t_device, buf, mem, 0);

    return mem;
}

static void
init_buffers(const struct test_params *p, struct test_resources *r)
{
    r->buf_size = p->img_w * p->img_h * bpp;

    VkBufferUsageFlags buf_a_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkBufferUsageFlags buf_b_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkBufferUsageFlags buf_c_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    r->buf_a = qoCreateBuffer(t_device, .size = r->buf_size, .usage = buf_a_usage);
    r->buf_b = qoCreateBuffer(t_device, .size = r->buf_size, .usage = buf_b_usage);
    r->buf_c = qoCreateBuffer(t_device, .size = r->buf_size, .usage = buf_c_usage);

    r->mem_a = alloc_mem_for_buf(r->buf_a);
    r->mem_b = alloc_mem_for_buf(r->buf_b);
    r->mem_c = alloc_mem_for_buf(r->buf_c);

    r->ptr_a = qoMapMemory(t_device, r->mem_a, 0, r->buf_size, 0);
    r->ptr_b = qoMapMemory(t_device, r->mem_b, 0, r->buf_size, 0);
    r->ptr_c = qoMapMemory(t_device, r->mem_c, 0, r->buf_size, 0);

    for (uint64_t i = 0; i < r->buf_size; i++) {
        r->ptr_a[i] = 0xAA;
        r->ptr_b[i] = 0xBB;
        r->ptr_c[i] = 0xCC;
    }

    VkBufferUsageFlags buf_indirect_usage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    uint64_t buf_indirect_size = sizeof(VkCopyMemoryIndirectCommandKHR);
    r->buf_indirect = qoCreateBuffer(t_device,
                                     .size = buf_indirect_size,
                                     .usage = buf_indirect_usage);
    r->mem_indirect = alloc_mem_for_buf(r->buf_indirect);

    VkBufferDeviceAddressInfo addr_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = r->buf_a,
    };
    r->buf_a_addr = vkGetBufferDeviceAddress(t_device, &addr_info);

    addr_info.buffer = r->buf_b;
    r->buf_b_addr = vkGetBufferDeviceAddress(t_device, &addr_info);

    addr_info.buffer = r->buf_indirect;
    r->buf_indirect_addr = vkGetBufferDeviceAddress(t_device, &addr_info);

    r->ptr_indirect = qoMapMemory(t_device, r->mem_indirect, 0,
                                  buf_indirect_size, 0);
    r->ptr_indirect->srcAddress = r->buf_a_addr;
    r->ptr_indirect->dstAddress = r->buf_b_addr;
    r->ptr_indirect->size = r->buf_size;
}

static void
init_image(const struct test_params *p, struct test_resources *r)
{
    r->img = qoCreateImage(t_device,
                           .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           .format = format,
                           .tiling = (p->use_tiling ? VK_IMAGE_TILING_OPTIMAL :
                                                      VK_IMAGE_TILING_LINEAR),
                           .extent = (VkExtent3D) { .width = p->img_w,
                                                    .height = p->img_h,
                                                    .depth = 1 });
    r->mem_img = qoAllocImageMemory(t_device, r->img,
                                    .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    qoBindImageMemory(t_device, r->img, r->mem_img, 0);
}

static void
program_cmd_buffer(const struct test_params *p, struct test_resources *r,
                   VkCommandBuffer cmd_buf)
{
    VkImageSubresourceRange img_subres = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier img_barrier1 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = r->img,
        .subresourceRange = img_subres,
    };
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, /* dependencyFlags */
                         0, NULL, /* memory barriers */
                         0, NULL, /* buffer barriers */
                         1, &img_barrier1);

    VkCopyMemoryIndirectInfoKHR ind_copy_info = {
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_INDIRECT_INFO_KHR,
        .srcCopyFlags = VK_ADDRESS_COPY_DEVICE_LOCAL_BIT_KHR,
        .dstCopyFlags = VK_ADDRESS_COPY_DEVICE_LOCAL_BIT_KHR,
        .copyCount = 1,
        .copyAddressRange = (VkStridedDeviceAddressRangeKHR) {
            .address = r->buf_indirect_addr,
            .size = sizeof(VkCopyMemoryIndirectCommandKHR),
            .stride = sizeof(VkCopyMemoryIndirectCommandKHR),
        },
    };
    copy_mem_indirect_fn(cmd_buf, &ind_copy_info);

    VkBufferMemoryBarrier buf_barrier1 = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = r->buf_b,
        .offset = 0,
        .size = r->buf_size,
    };
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, /* dependencyFlags */
                         0, NULL, /* memory barriers */
                         1, &buf_barrier1, /* buffer barriers */
                         0, NULL);

    VkBufferImageCopy img_copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = (VkImageSubresourceLayers) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
        },
        .imageOffset = (VkOffset3D) { .x = 0, .y = 0, .z = 0 },
        .imageExtent = (VkExtent3D) { .width = p->img_w,
                                      .height = p->img_h,
                                      .depth = 1 },
    };
    vkCmdCopyBufferToImage(cmd_buf, r->buf_b, r->img, VK_IMAGE_LAYOUT_GENERAL,
                           1, &img_copy_region);

    VkImageMemoryBarrier img_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = r->img,
        .subresourceRange = img_subres,
    };
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, /* dependencyFlags */
                         0, NULL, /* memory barriers */
                         0, NULL, /* buffer barriers */
                         1, &img_barrier2);

    vkCmdCopyImageToBuffer(cmd_buf, r->img, VK_IMAGE_LAYOUT_GENERAL, r->buf_c,
                           1, &img_copy_region);

    VkBufferMemoryBarrier buf_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = r->buf_c,
        .offset = 0,
        .size = r->buf_size,
    };
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0, /* dependencyFlags */
                         0, NULL, /* memory barriers */
                         1, &buf_barrier2, /* buffer barriers */
                         0, NULL);

    qoEndCommandBuffer(cmd_buf);
}

static void
check_results(struct test_resources *r)
{
    /* The happy case only does the memcmp(), which is *much* faster. */
    if (memcmp(r->ptr_a, r->ptr_c, r->buf_size) == 0)
        return; /* Success! */

    /* We know there's a failure, let's go through the slow path to figure out
     * why.
     */
    for (uint64_t i = 0; i < r->buf_size; i++) {
        switch (r->ptr_c[i]) {
        case 0xAA:
            /* This is what we were expecting: 'a' got copied into 'b', then
             * into 'img' and then to 'c'.
             */
            break;
        case 0xBB:
            /* The 'a -> b' copy was ignored or bypassed: the original contents
             * of 'b' were copied to 'img' and then 'c'.
             *
             * This is the original bug we wrote this test for: we were not
             * flushing the data cache, so the 'b -> c' copy read the old 'b'
             * bits using the sampler.
             *
             * Another possibility could be that the 'a -> b' copy did not copy
             * anything at all.
             */
            t_failf("'a -> b' copy wrong or needs flushing");
            break;
        case 0xCC:
            t_failf("unexpected: none of the commands wrote to the 'c' buffer!");
            break;
        default:
            t_failf("unexpected: value read is 0x%x", r->ptr_c[i]);
            break;
        }
    }
}

static void
regression_f33b98ff(void)
{
    const struct test_params *p = t_user_data;
    struct test_resources r = {};

    check_test_requirements();

    copy_mem_indirect_fn = (PFN_vkCmdCopyMemoryIndirectKHR)
        vkGetDeviceProcAddr(t_device, "vkCmdCopyMemoryIndirectKHR");
    t_assert(copy_mem_indirect_fn != NULL);

    init_buffers(p, &r);
    init_image(p, &r);
    program_cmd_buffer(p, &r, t_cmd_buffer);

    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    check_results(&r);

    t_pass();
}

/* This is here to help with the token pasting madness. */
static const bool tiling = true;
static const bool linear = false;
static const enum test_queue_setup graphics = QUEUE_SETUP_GFX_AND_COMPUTE;
static const enum test_queue_setup compute = QUEUE_SETUP_COMPUTE;
static const enum test_queue_setup transfer = QUEUE_SETUP_TRANSFER;

#define TEST_DEF__(dim, use_tiling_val, queue) \
    test_define { \
        .name = "bug.regression-f33b98ff." #dim "." #use_tiling_val "." #queue, \
        .start = regression_f33b98ff, \
        .queue_setup = queue, \
        .no_image = true, \
        .api_version = VK_API_VERSION_1_3, \
        .user_data = &use_tiling_val ## _ ## dim ## x ## dim, \
    };

#define TEST_DEF_(dim, use_tiling_val) \
    static struct test_params use_tiling_val ## _ ## dim ## x ## dim = { \
        .img_w = (dim), \
        .img_h = (dim), \
        .use_tiling = (use_tiling_val), \
    }; \
    TEST_DEF__(dim, use_tiling_val, graphics); \
    TEST_DEF__(dim, use_tiling_val, compute); \
    TEST_DEF__(dim, use_tiling_val, transfer);

#define TEST_DEF(dim) \
    TEST_DEF_(dim, tiling); \
    TEST_DEF_(dim, linear);

TEST_DEF(4);
TEST_DEF(1024);
TEST_DEF(4096);
