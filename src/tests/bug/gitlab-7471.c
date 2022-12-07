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

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/7471
 *
 * The issue is a crash in Anv when computing SCISSOR values in a
 * secondary command buffer which has a 0x0 render area due to the
 * render pass being started/ended in the primary command buffer.
 */

static VkBuffer
make_vbo(void)
{
    size_t vbo_size = 4 * sizeof(float);

    VkBuffer vbo =
        qoCreateBuffer(t_device, .size = vbo_size,
                       .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkDeviceMemory vbo_mem = qoAllocBufferMemory(t_device, vbo,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    qoBindBufferMemory(t_device, vbo, vbo_mem, /*offset*/ 0);

    return vbo;
}

static VkPipelineLayout
create_pipeline_layout(void)
{
    return qoCreatePipelineLayout(t_device);
}

static VkPipeline
create_pipeline(VkRenderPass pass, VkPipelineLayout layout)
{
    return qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .dynamicStates = (1 << VK_DYNAMIC_STATE_VIEWPORT) |
                             (1 << VK_DYNAMIC_STATE_SCISSOR),
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .renderPass = pass,
            .layout = layout,
            .subpass = 0,
        }});
}

static VkRenderPass
create_renderpass(void)
{
  return qoCreateRenderPass(t_device,
                            .attachmentCount = 1,
                            .pAttachments = (VkAttachmentDescription[]) {
                              {
                                .format = VK_FORMAT_B8G8R8A8_UNORM,
                                .samples = VK_SAMPLE_COUNT_1_BIT,
                                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
                                .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
                              },
                            },
                            .subpassCount = 1,
                            .pSubpasses = (VkSubpassDescription[]) {
                              {
                                QO_SUBPASS_DESCRIPTION_DEFAULTS,
                                .colorAttachmentCount = 1,
                                .pColorAttachments = (VkAttachmentReference[]) {
                                  {
                                    .attachment = 0,
                                    .layout = VK_IMAGE_LAYOUT_GENERAL,
                                  },
                                },
                              }
                            });
}

static void
test_gitlab_7471(void)
{
    VkPipelineLayout pipeline_layout = create_pipeline_layout();
    VkRenderPass renderpass = create_renderpass();
    VkPipeline pipeline = create_pipeline(renderpass, pipeline_layout);
    VkBuffer vbo = make_vbo();
    VkCommandBuffer cmd = qoAllocateCommandBuffer(t_device, t_cmd_pool,
                                                  .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    qoBeginCommandBuffer(cmd,
                         .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
                         .pInheritanceInfo = (&(VkCommandBufferInheritanceInfo) {
                             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                             .renderPass = renderpass,
                             .framebuffer = VK_NULL_HANDLE,
                           }));

    vkCmdSetViewport(cmd, 0, 1,
                     (VkViewport[]) {
                       {
                         .x        = -100.0f,
                         .y        = -100.0f,
                         .width    = 100.0f,
                         .height   = 100.0f,
                         .minDepth = 0.0f,
                         .maxDepth = 1.0f,
                       },
                     });
    vkCmdSetScissor(cmd, 0, 1,
                    (VkRect2D[]) {
                      {
                        .offset = {
                          .x = 0,
                          .y = 0,
                        },
                        .extent = {
                          .width = 100,
                          .height = 100,
                        },
                      },
                    });
    vkCmdBindVertexBuffers(cmd, 0, 2,
                           (VkBuffer[]) { vbo, vbo },
                           (VkDeviceSize[]) { 0, 0 });

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(cmd, 1, 1, 0, 0);

    qoEndCommandBuffer(cmd);

    t_end(TEST_RESULT_PASS);
}

test_define {
    .name = "bug.gitlab.7471",
    .start = test_gitlab_7471,
    .no_image = true,
};
