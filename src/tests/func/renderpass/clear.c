// Copyright 2015 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including the next
// paragraph) shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

/// @file
/// @brief Test VK_ATTACHMENT_LOAD_OP_CLEAR
///
/// TODO: Test a render pass in which some attachments have a normalized
/// format and some have an integer format.
/// TODO: Test a render pass that contains multiple color attachments and
/// multiple depthstencil attachments.
/// TODO: Test attachments where baseMipLevel > 0.
/// TODO: Test attachments where baseArraySlice > 0.
/// TODO: Test attachments of 1D, 3D, and cube map images.
/// TODO: Test multisampled attachments.

#include <math.h>

#include "tapi/t.h"
#include "util/cru_format.h"

static void
check_requirements(uint32_t num_color_attachments)
{
    if (num_color_attachments >
        t_physical_dev_props->limits.maxColorAttachments)
    {
        t_skipf("test requires %d color attachments, but physical device "
                "supports only %d", num_color_attachments,
                t_physical_dev_props->limits.maxColorAttachments);
    }
}

#define ALIGN(val, align) (((val) + (align) - 1) & ~((align) - 1))

static void
test_color_view_one(void)
{
    static const uint32_t num_attachments = 4;
    static const uint32_t width = 64;
    static const uint32_t height = 64;

    VkFormat formats[4] =
            { VK_FORMAT_R32_UINT, VK_FORMAT_R32_SFLOAT,
              VK_FORMAT_R32_UINT, VK_FORMAT_R32_SFLOAT };
    VkFormat view_formats[4] =
            { VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT,
              VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT };
    VkImageFormatListCreateInfoKHR list = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
            .viewFormatCount = 2,
            .pViewFormats = &view_formats[0],
    };
    VkImageFormatListCreateInfoKHR *fmt_list[4] = {NULL, NULL, &list, &list};

    VkClearValue clear_values[num_attachments];

    /* Prior to the mesa commit 85d0bec9616bc1ffa8e4ab5e7c5d12ff4e414872
     * ("anv: Be more careful about fast-clear colors"), anv checked all
     * components of the clear color regardless of the view format. Memset the
     * components to zero for reproducibility prior to that commit.
     */
    memset(clear_values, 0, sizeof(VkClearValue) * num_attachments);
    clear_values[0].color.float32[0] = 1.0f;
    clear_values[1].color.uint32[0] = 1;
    clear_values[2].color.float32[0] = 1.0f;
    clear_values[3].color.uint32[0] = 1;

    VkImage images[num_attachments];
    VkImageView att_views[num_attachments];
    VkAttachmentDescription att_descs[num_attachments];
    VkAttachmentReference att_references[num_attachments];


    VkBuffer dest_buffers[num_attachments];
    cru_image_t *ref_images[num_attachments];
    cru_image_t *actual_images[num_attachments];

    check_requirements(num_attachments);

    for (uint32_t i = 0; i < num_attachments; ++i) {
        const cru_format_info_t *format_info = t_format_info(formats[i]);

        images[i] = qoCreateImage(t_device,
            .pNext = fmt_list[i],
            .imageType = VK_IMAGE_TYPE_2D,
            .format = formats[i],
            .mipLevels = 1,
            .arrayLayers = 1,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1,
            },
            .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        VkDeviceMemory mem = qoAllocImageMemory(t_device, images[i],
            .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        qoBindImageMemory(t_device, images[i], mem, 0);

        att_views[i] = qoCreateImageView(t_device,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = view_formats[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            });

        att_descs[i] = (VkAttachmentDescription) {
            .format = view_formats[i],
            .samples = 1,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        };

        att_references[i] = (VkAttachmentReference) {
            .attachment = i,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        size_t dest_buffer_size = format_info->cpp * width * height;

        dest_buffers[i] = qoCreateBuffer(t_device,
            .size = dest_buffer_size,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        VkDeviceMemory dest_buffer_mem =
            qoAllocBufferMemory(t_device, dest_buffers[i],
                .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        qoBindBufferMemory(t_device, dest_buffers[i], dest_buffer_mem,
                           /*offset*/ 0);

        void *dest_buffer_map = qoMapMemory(t_device, dest_buffer_mem,
            /*offset*/ 0, dest_buffer_size, /*flags*/ 0);

        actual_images[i] = t_new_cru_image_from_pixels(dest_buffer_map,
            formats[i], width, height);

        void *ref_image_mem = xmalloc(dest_buffer_size);
        t_cleanup_push_free(ref_image_mem);

        ref_images[i] = t_new_cru_image_from_pixels(ref_image_mem,
                formats[i], width, height);

        for (uint32_t j = 0; j < width * height; ++j) {
            uint32_t *pixel_u32 = ref_image_mem + format_info->cpp * j;
            *pixel_u32 = clear_values[i].color.uint32[0];
        }
    }

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = num_attachments,
        .pAttachments = att_descs,
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = num_attachments,
                .pColorAttachments = att_references,
            },
        },
        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        });

    VkFramebuffer fb = qoCreateFramebuffer(t_device,
        .renderPass = pass,
        .attachmentCount = num_attachments,
        .pAttachments = att_views,
        .width = width,
        .height = height,
        .layers = 1);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = fb,
            .renderArea = {{0, 0}, {width, height}},
            .clearValueCount = num_attachments,
            .pClearValues = clear_values,
        },
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(t_cmd_buffer);

    for (uint32_t i = 0; i < num_attachments; ++i) {

        vkCmdCopyImageToBuffer(t_cmd_buffer, images[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dest_buffers[i],
            /*regionCount*/ 1,
            &(VkBufferImageCopy) {
                .bufferOffset = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer= 0,
                    .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { width, height, 1 },
            });

        vkCmdPipelineBarrier(t_cmd_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .buffer = dest_buffers[i],
                .size = VK_WHOLE_SIZE,
            }, 0, NULL);
    }


    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    test_result_t result = TEST_RESULT_PASS;

    for (uint32_t i = 0; i < num_attachments; ++i) {
        t_dump_image_f(ref_images[i], "attachment%02d.ref.png", i);
        t_dump_image_f(actual_images[i], "attachment%02d.actual.png", i);

        if (!cru_image_compare(ref_images[i], actual_images[i])) {
            result = TEST_RESULT_FAIL;
        }
    }

    t_end(result);
}
static void
test_color8(void)
{
    static const uint32_t num_attachments = 8;
    static const uint32_t width = 64;
    static const uint32_t height = 64;

    VkFormat formats[num_attachments];
    VkImage images[num_attachments];
    VkImageView att_views[num_attachments];
    VkAttachmentDescription att_descs[num_attachments];
    VkAttachmentReference att_references[num_attachments];
    VkClearValue clear_values[num_attachments];

    VkBuffer dest_buffers[num_attachments];
    cru_image_t *ref_images[num_attachments];
    cru_image_t *actual_images[num_attachments];

    check_requirements(num_attachments);

    for (uint32_t i = 0; i < num_attachments; ++i) {
        formats[i] = VK_FORMAT_R8G8B8A8_UNORM;

        const cru_format_info_t *format_info = t_format_info(formats[i]);

        images[i] = qoCreateImage(t_device,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = formats[i],
            .mipLevels = 1,
            .arrayLayers = 1,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1,
            },
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        VkDeviceMemory mem = qoAllocImageMemory(t_device, images[i],
            .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        qoBindImageMemory(t_device, images[i], mem, 0);

        att_views[i] = qoCreateImageView(t_device,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = formats[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            });

        att_descs[i] = (VkAttachmentDescription) {
            .format = formats[i],
            .samples = 1,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        };

        att_references[i] = (VkAttachmentReference) {
            .attachment = i,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        // Don't use black for any clear value. That would allow the test to
        // falsely pass.
        clear_values[i] = (VkClearValue) {
            .color = {
                .float32 = {
                    ((i % 5) + 1.0) / 5.0,
                    ((i % 9) + 1.0) / 9.0,
                    ((i % 3) + 1.0) / 3.0,
                    ((i % 7) + 1.0) / 7.0,
                },
            },
        };

        // Converting normalized float to normalized unorm8 requires rounding
        // to the nearest int.
        const uint8_t clear_value_u8[] = {
            roundf(255 * clear_values[i].color.float32[0]),
            roundf(255 * clear_values[i].color.float32[1]),
            roundf(255 * clear_values[i].color.float32[2]),
            roundf(255 * clear_values[i].color.float32[3]),
        };

        size_t dest_buffer_size = format_info->cpp * width * height;

        dest_buffers[i] = qoCreateBuffer(t_device,
            .size = dest_buffer_size,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        VkDeviceMemory dest_buffer_mem =
            qoAllocBufferMemory(t_device, dest_buffers[i],
                .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        qoBindBufferMemory(t_device, dest_buffers[i], dest_buffer_mem,
                           /*offset*/ 0);

        void *dest_buffer_map = qoMapMemory(t_device, dest_buffer_mem,
            /*offset*/ 0, dest_buffer_size, /*flags*/ 0);

        actual_images[i] = t_new_cru_image_from_pixels(dest_buffer_map,
            formats[i], width, height);

        void *ref_image_mem = xmalloc(dest_buffer_size);
        t_cleanup_push_free(ref_image_mem);

        ref_images[i] = t_new_cru_image_from_pixels(ref_image_mem,
                formats[i], width, height);

        for (uint32_t j = 0; j < width * height; ++j) {
            uint8_t *pixel_u8 = ref_image_mem + format_info->cpp * j;

            pixel_u8[0] = clear_value_u8[0];
            pixel_u8[1] = clear_value_u8[1];
            pixel_u8[2] = clear_value_u8[2];
            pixel_u8[3] = clear_value_u8[3];
        }
    }

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = num_attachments,
        .pAttachments = att_descs,
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = num_attachments,
                .pColorAttachments = att_references,
            },
        },
        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        });

    VkFramebuffer fb = qoCreateFramebuffer(t_device,
        .renderPass = pass,
        .attachmentCount = num_attachments,
        .pAttachments = att_views,
        .width = width,
        .height = height,
        .layers = 1);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = fb,
            .renderArea = {{0, 0}, {width, height}},
            .clearValueCount = num_attachments,
            .pClearValues = clear_values,
        },
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(t_cmd_buffer);

    for (uint32_t i = 0; i < num_attachments; ++i) {

        vkCmdCopyImageToBuffer(t_cmd_buffer, images[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dest_buffers[i],
            /*regionCount*/ 1,
            &(VkBufferImageCopy) {
                .bufferOffset = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer= 0,
                    .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { width, height, 1 },
            });

        vkCmdPipelineBarrier(t_cmd_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .buffer = dest_buffers[i],
                .size = VK_WHOLE_SIZE,
            }, 0, NULL);
    }

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    test_result_t result = TEST_RESULT_PASS;

    for (uint32_t i = 0; i < num_attachments; ++i) {
        t_dump_image_f(ref_images[i], "attachment%02d.ref.png", i);
        t_dump_image_f(actual_images[i], "attachment%02d.actual.png", i);

        if (!cru_image_compare(ref_images[i], actual_images[i])) {
            result = TEST_RESULT_FAIL;
        }
    }

    t_end(result);
}

static void
test_color8_shared_memory(void)
{
    static const uint32_t num_attachments = 3;
    static const uint32_t width = 64;
    static const uint32_t height = 64;

    VkFormat formats[num_attachments];
    VkImage images[num_attachments];
    VkImageView att_views[num_attachments];
    VkAttachmentDescription att_descs[num_attachments];
    VkAttachmentReference att_references[num_attachments];
    VkClearValue clear_values[num_attachments];

    VkBuffer dest_buffers[num_attachments];
    cru_image_t *ref_images[num_attachments];
    cru_image_t *actual_images[num_attachments];

    check_requirements(num_attachments);

    for (uint32_t i = 0; i < num_attachments; ++i) {
        formats[i] = VK_FORMAT_R8G8B8A8_UNORM;

        images[i] = qoCreateImage(t_device,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = formats[i],
            .mipLevels = 1,
            .arrayLayers = 1,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1,
            },
            .samples = 1,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    }

    VkMemoryRequirements mem_reqs =
        qoGetImageMemoryRequirements(t_device, images[0]);
    mem_reqs.size = 3 * ALIGN(mem_reqs.size, mem_reqs.alignment);

    uint32_t type_index = UINT32_MAX;
    const VkPhysicalDeviceMemoryProperties *props = t_physical_dev_mem_props;
    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        if (mem_reqs.memoryTypeBits & (1 << i)) {
            type_index = i;
            break;
        }
    }
    assert(type_index < props->memoryTypeCount);

    VkDeviceMemory mem;
    VkResult res = vkAllocateMemory(t_device, &(VkMemoryAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = type_index,
        },
        NULL,
        &mem);
    assert(res == VK_SUCCESS);

    for (uint32_t i = 0; i < num_attachments; ++i) {
        qoBindImageMemory(t_device, images[i], mem, i * (mem_reqs.size / 3));

        att_views[i] = qoCreateImageView(t_device,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = formats[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            });

        att_descs[i] = (VkAttachmentDescription) {
            .format = formats[i],
            .samples = 1,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        };

        att_references[i] = (VkAttachmentReference) {
            .attachment = i,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        clear_values[i] = (VkClearValue) {
            .color = {
                .float32 = {
                    i == 0 ? 1.0f : 0.0f,
                    i == 1 ? 1.0f : 0.0f,
                    i == 2 ? 1.0f : 0.0f,
                    1.0,
                },
            },
        };

        // Converting normalized float to normalized unorm8 requires rounding
        // to the nearest int.
        const uint8_t clear_value_u8[] = {
            roundf(255 * clear_values[i].color.float32[0]),
            roundf(255 * clear_values[i].color.float32[1]),
            roundf(255 * clear_values[i].color.float32[2]),
            roundf(255 * clear_values[i].color.float32[3]),
        };

        const cru_format_info_t *format_info = t_format_info(formats[i]);

        size_t dest_buffer_size = format_info->cpp * width * height;

        dest_buffers[i] = qoCreateBuffer(t_device,
            .size = dest_buffer_size,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        VkDeviceMemory dest_buffer_mem =
            qoAllocBufferMemory(t_device, dest_buffers[i],
                .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        qoBindBufferMemory(t_device, dest_buffers[i], dest_buffer_mem,
                           /*offset*/ 0);

        void *dest_buffer_map = qoMapMemory(t_device, dest_buffer_mem,
            /*offset*/ 0, dest_buffer_size, /*flags*/ 0);

        actual_images[i] = t_new_cru_image_from_pixels(dest_buffer_map,
            formats[i], width, height);

        void *ref_image_mem = xmalloc(dest_buffer_size);
        t_cleanup_push_free(ref_image_mem);

        ref_images[i] = t_new_cru_image_from_pixels(ref_image_mem,
                formats[i], width, height);

        for (uint32_t j = 0; j < width * height; ++j) {
            uint8_t *pixel_u8 = ref_image_mem + format_info->cpp * j;

            pixel_u8[0] = clear_value_u8[0];
            pixel_u8[1] = clear_value_u8[1];
            pixel_u8[2] = clear_value_u8[2];
            pixel_u8[3] = clear_value_u8[3];
        }
    }

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = num_attachments,
        .pAttachments = att_descs,
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = num_attachments,
                .pColorAttachments = att_references,
            },
        },
        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        });

    VkFramebuffer fb = qoCreateFramebuffer(t_device,
        .renderPass = pass,
        .attachmentCount = num_attachments,
        .pAttachments = att_views,
        .width = width,
        .height = height,
        .layers = 1);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = fb,
            .renderArea = {{0, 0}, {width, height}},
            .clearValueCount = num_attachments,
            .pClearValues = clear_values,
        },
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(t_cmd_buffer);

    for (uint32_t i = 0; i < num_attachments; ++i) {

        vkCmdCopyImageToBuffer(t_cmd_buffer, images[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dest_buffers[i],
            /*regionCount*/ 1,
            &(VkBufferImageCopy) {
                .bufferOffset = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer= 0,
                    .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { width, height, 1 },
            });

        vkCmdPipelineBarrier(t_cmd_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .buffer = dest_buffers[i],
                .size = VK_WHOLE_SIZE,
            }, 0, NULL);
    }

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    test_result_t result = TEST_RESULT_PASS;

    for (uint32_t i = 0; i < num_attachments; ++i) {
        t_dump_image_f(ref_images[i], "attachment%02d.ref.png", i);
        t_dump_image_f(actual_images[i], "attachment%02d.actual.png", i);

        if (!cru_image_compare(ref_images[i], actual_images[i])) {
            result = TEST_RESULT_FAIL;
        }
    }

    t_end(result);
}

static void
test_color_render_area(void)
{
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                .format = format,
                .samples = 1,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
            },
        },
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = (VkAttachmentReference[]) {
                    {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_GENERAL,
                    },
                },
            },
        });

    // Clear the whole image to green.
    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = t_framebuffer,
            .renderArea = {
                { 0, 0 },
                { t_width, t_height },
            },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 0, 1, 0, 1 }}},
            },
        },
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(t_cmd_buffer);

    // Clear a subregion to blue.
    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = t_framebuffer,
            .renderArea = {
                { t_width / 4, t_height / 4 },
                { t_width / 2, t_height / 2 },
            },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 0, 0, 1, 1 }}},
            },
        },
        VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(t_cmd_buffer);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

/// Create a render pass that clears each attachment to a unique clear color
/// using VK_ATTACHMENT_LOAD_OP_CLEAR.  Submit a command buffer that trivially
/// begins then ends the render pass.  Then confirm that each attachment is
/// filled with the expected clear color.
test_define {
    .name = "func.renderpass.clear.color08",
    .start = test_color8,
    .no_image = true,
};

/// Create a render pass that clears a float view of an integer image to 1.0f
/// and an integer view of a float image to 1 using
/// VK_ATTACHMENT_LOAD_OP_CLEAR.  Submit a command buffer that trivially
/// begins then ends the render pass.  Then confirm that each attachment is
/// filled with the expected clear color. This tests a fast-clear case on
/// gen7-8 (https://bugs.freedesktop.org/show_bug.cgi?id=105826).
test_define {
    .name = "func.renderpass.clear.color-view-one",
    .start = test_color_view_one,
    .no_image = true,
};

/// Create a render pass that clears each attachment to a unique clear
/// color using VK_ATTACHMENT_LOAD_OP_CLEAR. Submit a command buffer
/// that trivially begins then ends the render pass. Then confirm that
/// each attachment is filled with the expected clear color. All the
/// attachments are stored within the same memory object (allowing
/// detection of reading/writing at the wrong offset in memory).
test_define {
    .name = "func.renderpass.clear.color08-shared-memory",
    .start = test_color8_shared_memory,
    .no_image = true,
};

/// Submit two renderpasses that draw to the same framebuffer. The first
/// clears the whole framebuffer. The second clears a subrect of the
/// framebuffer to a different color using VkRenderPassBeginInfo::renderArea.
test_define {
    .name = "func.renderpass.clear.color-render-area",
    .start = test_color_render_area,
};
