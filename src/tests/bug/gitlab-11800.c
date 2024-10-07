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

#include <inttypes.h>
#include <stdio.h>
#include "tapi/t.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/11800
 *
 * The issue about propagating conditional modifiers on float
 * comparisons. The special handling on NaN values forbids us to
 * propagate.
 */

#include "tapi/t.h"

#include "src/tests/bug/gitlab-11800-spirv.h"

static VkPipeline
create_pipeline(VkDevice device, VkPipelineLayout pipeline_layout)
{
    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        layout(location = 0) in vec4 a_position;
        layout(location = 1) in vec4 a_color;
        layout(location = 0) out vec4 v_color;
        void main()
        {
            gl_Position = a_position;
            v_color = a_color;
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;
        layout(location = 0) in vec4 v_color;
        layout(set = 0, binding = 0) uniform block1 {
            float value;
        } u;

        void main()
        {
            if (u.value >= 0)
                discard;
            f_color = v_color;
        }
    );

    VkPipelineVertexInputStateCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 2,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
            {
                .binding = 0,
                .stride = 16,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
            {
                .binding = 1,
                .stride = 0,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
        },
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = 0,
            },
            {
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = 0,
            },
        },
    };

    return qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .vertexShader = vs,
            .fragmentShader = fs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &vi_create_info,
            .flags = 0,
            .layout = pipeline_layout,
            .renderPass = t_render_pass,
            .subpass = 0,
        }});
}

static void
test(void)
{
    VkDescriptorSetLayout set_layout;

    set_layout = qoCreateDescriptorSetLayout(t_device,
            .bindingCount = 1,
            .pBindings = (VkDescriptorSetLayoutBinding[]) {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
                },
            });

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout);

    VkPipeline pipeline = create_pipeline(t_device, pipeline_layout);

    VkDescriptorSet set;
    VkResult result = vkAllocateDescriptorSets(t_device,
        &(VkDescriptorSetAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = t_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &set_layout,
        }, &set);
    t_assert(result == VK_SUCCESS);

    /* Generate a NaN value */
    float uniform_data;
    memset(&uniform_data, -1, sizeof(uniform_data));

    VkBuffer uniform_buffer = qoCreateBuffer(t_device, .size = sizeof(uniform_data));

    VkDeviceMemory uniform_mem = qoAllocBufferMemory(t_device, uniform_buffer,
						     .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    qoBindBufferMemory(t_device, uniform_buffer,
                       uniform_mem, /*offset*/ 0);

    void *ubo_map = qoMapMemory(t_device, uniform_mem, /*offset*/ 0,
                                sizeof(uniform_data), /*flags*/ 0);
    *((float*)ubo_map) = uniform_data;

    static const float vertex_data[] = {
        // Triangle coordinates
        -0.5, -0.5, 0.0, 1.0,
         0.5, -0.5, 0.0, 1.0,
         0.0,  0.5, 0.0, 1.0,

         // Color
         1.0,  0.0, 1.0, 1.0,
    };

    VkBuffer vertex_buffer = qoCreateBuffer(t_device,
        .size = sizeof(vertex_data),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkDeviceMemory vertex_mem = qoAllocBufferMemory(t_device, vertex_buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    memcpy(qoMapMemory(t_device, vertex_mem, /*offset*/ 0,
                       sizeof(vertex_data), /*flags*/ 0),
           vertex_data,
           sizeof(vertex_data));

    qoBindBufferMemory(t_device, vertex_buffer, vertex_mem,
                       /*offset*/ 0);

    vkUpdateDescriptorSets(t_device,
        1, /* writeCount */
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = (VkDescriptorBufferInfo[]) {
                    {
                        .buffer = uniform_buffer,
                        .offset = 0,
                        .range = VK_WHOLE_SIZE,
                    }
                },
            },
        }, 0, NULL);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .renderPass = t_render_pass,
            .framebuffer = t_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 1.0, 0.0, 0.0, 1.0 } } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 2,
                           (VkBuffer[]) { vertex_buffer, vertex_buffer },
                           (VkDeviceSize[]) { 0, 3 * 4 * sizeof(float) });
    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, 0, 1,
                            &set, 0, NULL);
    vkCmdDraw(t_cmd_buffer, /*vertexCount*/ 3, /*instanceCount*/ 1,
              /*firstVertex*/ 0, /*firstInstance*/ 0);
    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

test_define {
    .name = "bug.gitlab-11800",
    .start = test,
};
