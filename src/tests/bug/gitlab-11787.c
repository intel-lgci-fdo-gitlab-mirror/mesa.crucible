// Copyright 2024 Intel Corporation
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

#include "tapi/t.h"

#include "src/tests/bug/gitlab-11787-spirv.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/11787
 *
 * The issue about failing to render to depth image at non-zero mip level.
 */

// The depth value in fragment shader is 0.23f.
#define DEPTH_VALUE 0.23f

typedef struct test_params {
    VkFormat format;
    uint32_t mip_level;
} test_params_t;

static uint32_t
align(uint32_t value, size_t alignment)
{
    assert(value <= UINT32_MAX - (alignment - 1));
    assert(alignment != 0);
    // The alignment is power of 2
    assert((alignment & (alignment - 1)) == 0);
    uint32_t alignment_u32 = (uint32_t)alignment;
    return (value + (alignment_u32 - 1)) & ~(alignment_u32 - 1);
}

uint16_t float2uint16(float value)
{
    return (uint16_t)(value * (float)UINT16_MAX);
}

uint32_t get_pixel_bytes(VkFormat format, VkImageAspectFlags aspect)
{
    uint32_t bytes_per_pixel = 0;
    switch (format) {
        case VK_FORMAT_D16_UNORM: {
            assert(aspect == VK_IMAGE_ASPECT_DEPTH_BIT);
            bytes_per_pixel = 2;
            break;
        }
        case VK_FORMAT_D32_SFLOAT: {
            assert(aspect ==  VK_IMAGE_ASPECT_DEPTH_BIT);
            bytes_per_pixel = 4;
            break;
        }
        case VK_FORMAT_D32_SFLOAT_S8_UINT: {
            switch (aspect) {
                case VK_IMAGE_ASPECT_DEPTH_BIT:
                    bytes_per_pixel = 4;
                    break;
                case VK_IMAGE_ASPECT_STENCIL_BIT:
                    bytes_per_pixel = 1;
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return bytes_per_pixel;
}

static uint32_t
get_buffer_size_for_image_copy(uint32_t width,
                               uint32_t height,
                               uint32_t depth,
                               VkFormat format,
                               VkImageAspectFlags aspect)
{
    const uint32_t  image_bytes_per_row_alignment = 256u;
    uint32_t bytes_per_pixel = get_pixel_bytes(format, aspect);

    uint32_t bytes_per_row =
        align(width * bytes_per_pixel, image_bytes_per_row_alignment);
    // Bytes per image before last array layer
    uint32_t bytes_per_image = bytes_per_row * height;

    uint32_t result = bytes_per_image* (depth - 1)
                    + (bytes_per_row * (height - 1)
                    + width * bytes_per_pixel);

    return align(result, 4);
}

static void
test()
{
    const test_params_t *params = t_user_data;

    static const uint32_t base_size = 2;

    VkImage depth_img = qoCreateImage(t_device,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = params->format,
        .mipLevels = params->mip_level + 1,
        .extent = {
            .width = base_size << params->mip_level,
            .height = base_size << params->mip_level,
            .depth = 1},
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    {
        VkDeviceMemory depth_img_mem = qoAllocImageMemory(t_device, depth_img,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        qoBindImageMemory(t_device, depth_img, depth_img_mem, 0);
    }

    VkImageView depth_img_view = qoCreateImageView(t_device,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = params->format,
        .image = depth_img,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = params->mip_level,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .pNext = &(VkImageViewUsageCreateInfo) {
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        });

    VkRenderPass render_pass = qoCreateRenderPass(t_device,
        .attachmentCount = 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                QO_ATTACHMENT_DESCRIPTION_DEFAULTS,
                .format = params->format,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
        },
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                QO_SUBPASS_DESCRIPTION_DEFAULTS,
                .pDepthStencilAttachment = &(VkAttachmentReference) {
                    .attachment = 0,
                    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                },
            },
        });

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device);

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device,
        t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .vertexShader = qoCreateShaderModuleGLSL(t_device, VERTEX,
                vec2 positions[6] = vec2[](
                    vec2(-1.0, 1.0),
                    vec2(1.0, -1.0),
                    vec2(-1.0, -1.0),
                    vec2(-1.0, 1.0),
                    vec2(1.0, -1.0),
                    vec2(1.0, 1.0)
                );

                void main()
                {
                    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
                }
            ),
            .fragmentShader = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
                layout(location = 0) out vec4 out_color;

                void main()
                {
                    gl_FragDepth = 0.23;
                    out_color = vec4(0.0, 1.0, 0.0, 1.0);
                }
            ),
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 0,
                .vertexAttributeDescriptionCount = 0,
            },
            .pViewportState = &(VkPipelineViewportStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .flags = 0,
                .viewportCount = 1,
                .pViewports = &(VkViewport) {
                    .x = 0.0,
                    .y = 0.0,
                    .width = (float)base_size,
                    .height = (float)base_size,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
                },
                .scissorCount = 1,
                .pScissors = &(VkRect2D) {
                    .offset = { 0, 0},
                    .extent = { base_size, base_size }
                },
            },
            .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
                QO_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO_DEFAULTS,
                .depthTestEnable = true,
                .depthWriteEnable = true,
                .depthBoundsTestEnable = false,
                .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            },
            .layout = pipeline_layout,
            .renderPass = render_pass,
            .subpass = 0,
        }}
    );

    VkFramebuffer framebuffer = qoCreateFramebuffer(t_device,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &depth_img_view,
        .width = base_size,
        .height = base_size
    );

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = framebuffer,
            .renderArea = { { 0, 0 }, { base_size, base_size } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .depthStencil = { .depth = 0.69, .stencil = 1 } },
            },
        },
        VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdDraw(t_cmd_buffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(t_cmd_buffer);

    vkCmdPipelineBarrier(t_cmd_buffer,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = depth_img,
            .subresourceRange.aspectMask =
                params->format == VK_FORMAT_D32_SFLOAT_S8_UINT ?
                    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
                    VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.baseMipLevel = params->mip_level,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        });

    uint32_t buffer_size = get_buffer_size_for_image_copy(
        base_size,
        base_size,
        1,
        params->format,
        VK_IMAGE_ASPECT_DEPTH_BIT);
    VkBuffer buffer = qoCreateBuffer(t_device,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = buffer_size);
    VkDeviceMemory buffer_mem = qoAllocBufferMemory(t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer, buffer_mem, 0);

    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 1,
        &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = buffer_size,
        }, 0, NULL);

    vkCmdCopyImageToBuffer(t_cmd_buffer,
	    depth_img, VK_IMAGE_LAYOUT_GENERAL, buffer, 1,
		&(VkBufferImageCopy) {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =  {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .mipLevel = params->mip_level,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent.width = base_size,
            .imageExtent.height = base_size,
            .imageExtent.depth = 1
        });

    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0, 0, NULL, 1,
        &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = buffer_size,
        }, 0, NULL);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    vkQueueWaitIdle(t_queue);

    if (params->format == VK_FORMAT_D16_UNORM) {
        uint16_t *result;
        vkMapMemory(t_device, buffer_mem, 0, buffer_size, 0, (void **)&result);
        t_assertf(result[0] == float2uint16(DEPTH_VALUE),
            "expected to be %u, but got %u",
            float2uint16(DEPTH_VALUE), result[0]);
    } else {
        float *result;
        vkMapMemory(t_device, buffer_mem, 0, buffer_size, 0, (void **)&result);
        t_assertf(result[0] == DEPTH_VALUE,
            "expected to be %f, but got %f",
            DEPTH_VALUE, result[0]);
    }
}

test_define {
    .name = "bug.gitlab-11787.d16",
    .start = test,
    .user_data = &(test_params_t) {
        .format = VK_FORMAT_D16_UNORM,
        .mip_level = 1,
    },
    .no_image = true,
};

test_define {
    .name = "bug.gitlab-11787.d32",
    .start = test,
    .user_data = &(test_params_t) {
        .format = VK_FORMAT_D32_SFLOAT,
        .mip_level = 1,
    },
    .no_image = true,
};

test_define {
    .name = "bug.gitlab-11787.d32s8",
    .start = test,
    .user_data = &(test_params_t) {
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .mip_level = 1,
    },
    .no_image = true,
};
