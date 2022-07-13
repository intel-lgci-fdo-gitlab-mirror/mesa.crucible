// Copyright 2022 Intel Corporation
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

#include <inttypes.h>
#include <stdio.h>
#include "tapi/t.h"

#include "src/tests/bug/gitlab-6680-spirv.h"

#define GET_DEVICE_FUNCTION_PTR(name) \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(t_device, #name); \
    t_assert(name != NULL);

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/6680
 *
 * This issue is a WaW hazard between the buffer clearing and the transform
 * feedback writes. On Intel HW, the later is not L3 coherent, so we need to
 * flush the clear out of L3 before the transform feedback writes.
 */

static void
test_gitlab_6680(void)
{
    static const uint32_t buffer_size = 256;

    t_require_ext("VK_EXT_transform_feedback");

    GET_DEVICE_FUNCTION_PTR(vkCmdBindTransformFeedbackBuffersEXT);
    GET_DEVICE_FUNCTION_PTR(vkCmdBeginTransformFeedbackEXT);
    GET_DEVICE_FUNCTION_PTR(vkCmdEndTransformFeedbackEXT);

    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        layout(location = 0) in vec2 a_position;
        layout(xfb_buffer = 0, xfb_offset = 0, xfb_stride = 8, location = 0) out vec2 out0;
        void main()
        {
            out0 = a_position;
        }
    );

    VkPipelineVertexInputStateCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
            {
                .binding = 0,
                .stride = 8,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
        },
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = 0,
            },
        },
    };

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = 0,
        .pAttachments = (VkAttachmentDescription[]) {
        },
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                QO_SUBPASS_DESCRIPTION_DEFAULTS,
                .colorAttachmentCount = 0,
            }
        });

    VkFramebuffer fb = qoCreateFramebuffer(
        t_device,
        .attachmentCount = 0,
        .pAttachments = (VkImageView[]) {
            },
        .renderPass = pass,
        .width = 32,
        .height = 32,
        .layers = 1);

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
            .vertexShader = vs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &vi_create_info,
            .flags = 0,
            .layout = qoCreatePipelineLayout(t_device, 0, NULL),
            .renderPass = pass,
            .subpass = 0,
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .rasterizerDiscardEnable = true,
            },
        }});

    /**/
    VkBuffer v_buffer = qoCreateBuffer(t_device,
                                       .size = buffer_size,
                                       .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    {
        VkDeviceMemory v_mem = qoAllocBufferMemory(t_device, v_buffer,
                                                   .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        qoBindBufferMemory(t_device, v_buffer, v_mem, 0);
        float *v_map = qoMapMemory(t_device, v_mem, 0, buffer_size, 0);
        for (uint32_t i = 0; i < buffer_size / sizeof(float); i++)
            v_map[i] = i;
    }

    /**/
    VkBuffer t_buffer = qoCreateBuffer(t_device,
                                       .size = buffer_size,
                                       .usage = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT |
                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkDeviceMemory t_mem = qoAllocBufferMemory(t_device, t_buffer,
                                               .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, t_buffer, t_mem, 0);

    /**/
    VkDeviceSize v_offset = 0;
    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 1, &v_buffer, &v_offset);
    VkDeviceSize t_offset = 0;
    vkCmdBindTransformFeedbackBuffersEXT(t_cmd_buffer, 0, 1, &t_buffer, &t_offset, NULL);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 1,
                         &(VkBufferMemoryBarrier) {
                             .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                             .srcAccessMask = 0,
                             .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                             .buffer = t_buffer,
                             .size = VK_WHOLE_SIZE,
                         }, 0, NULL);

    vkCmdFillBuffer(t_cmd_buffer, t_buffer, 0, buffer_size, 0xdeaddead);

    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT,
                         0, 0, NULL, 1,
                         &(VkBufferMemoryBarrier) {
                             .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                             .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                             .dstAccessMask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
                             .buffer = t_buffer,
                             .size = VK_WHOLE_SIZE,
                         }, 0, NULL);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = fb,
            .renderArea = {{0, 0}, {32, 32}},
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 1.0, 0.0, 0.0, 1.0 } } },
            },
        },
        VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBeginTransformFeedbackEXT(t_cmd_buffer, 0, 0, NULL, NULL);
    vkCmdDraw(t_cmd_buffer, buffer_size / sizeof(float) / 2, 1, 0, 0);
    vkCmdEndTransformFeedbackEXT(t_cmd_buffer, 0, 0, NULL, NULL);

    vkCmdEndRenderPass(t_cmd_buffer);

    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0, 0, NULL, 1,
                         &(VkBufferMemoryBarrier) {
                             .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                             .srcAccessMask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
                             .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                             .buffer = t_buffer,
                             .size = VK_WHOLE_SIZE,
                         }, 0, NULL);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    {
        float *t_mapf = qoMapMemory(t_device, t_mem, 0, buffer_size, 0);
        uint32_t *t_map32 = (uint32_t *)t_mapf;
        for (uint32_t i = 0; i < buffer_size / sizeof(float); i++)
            t_assert(t_mapf[i] == i);
    }

    t_end(TEST_RESULT_PASS);
}

test_define {
    .name = "bug.gitlab.6680",
    .start = test_gitlab_6680,
    .no_image = true,
};
