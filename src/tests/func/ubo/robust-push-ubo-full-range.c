// Copyright 2025 Calder Young
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
#include "util/misc.h"

#include "src/tests/func/ubo/robust-push-ubo-full-range-spirv.h"

#define UBO_BLOCK_SIZE 16
#define UBO_BLOCK_COUNT 128

static uint32_t
check_size_alignment(void)
{
    VkPhysicalDeviceRobustness2PropertiesEXT robustnessProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT
    };
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &robustnessProps
    };
    vkGetPhysicalDeviceProperties2(t_physical_dev, &props);
    return (uint32_t) robustnessProps.robustUniformBufferAccessSizeAlignment;
}

static VkPipeline
create_pipeline(VkDevice device,
                VkPipelineLayout pipeline_layout)
{
    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        void main()
        {
            vec2 pos = vec2(float(gl_VertexIndex & 1),
                            float(gl_VertexIndex >> 1));
            gl_Position = vec4(vec2(-1) + 2 * pos, 0.0f, 1.0f);
        }
    );

    assert(UBO_BLOCK_SIZE == 16);
    assert(UBO_BLOCK_COUNT == 128);
    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_control_flow_attributes : require

        layout(location = 0) out vec4 f_color;

        layout(set = 0, binding = 0) uniform block1 {
            vec4 data[128];
        } u;

        void main()
        {
            f_color = vec4(0.0);
            uint x = uint(gl_FragCoord.x);
            [[unroll]]
            for (uint i = 0; i < 128; ++i) {
                if (i == x) {
                    f_color = u.data[i];
                    return;
                }
            }
        }
    );

    VkPipelineVertexInputStateCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };

    VkDynamicState dynamicState = VK_DYNAMIC_STATE_VIEWPORT;
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        NULL,
        0u,
        1u,
        &dynamicState
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
            .pDynamicState = &dynamicStateCreateInfo,
        }});
}

static VkBuffer
create_buffer(void)
{
    const VkDeviceSize buffer_size = UBO_BLOCK_SIZE * UBO_BLOCK_COUNT;

    VkBuffer buffer = qoCreateBuffer(t_device, .size = buffer_size);

    VkDeviceMemory mem = qoAllocBufferMemory(t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    qoBindBufferMemory(t_device, buffer, mem, /*offset*/ 0);

    void *map = qoMapMemory(t_device, mem, /*offset*/ 0,
                            buffer_size, /*flags*/ 0);

    uint32_t offset = 0;

    const float colors[4] = {
        64.0f / 255.0f,
        128.0f / 255.0f,
        192.0f / 255.0f,
        255.0f / 255.0f,
    };

    for (unsigned i = 0; i < UBO_BLOCK_COUNT; ++i) {
        assert(offset + sizeof(colors) <= buffer_size);
        memcpy(map + offset, colors, sizeof(colors));
        offset += sizeof(colors);
    }

    return buffer;
}

static void
test(void)
{
    assert(t_width == UBO_BLOCK_COUNT && t_height == UBO_BLOCK_COUNT);

    uint32_t size_align = check_size_alignment();
    if (UBO_BLOCK_SIZE % size_align != 0)
        t_skipf("Test alignment of %u bytes is incompatible with the device's alignment of %u bytes",
                UBO_BLOCK_SIZE, size_align);

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(t_device,
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

    VkBuffer buffer = create_buffer();

    VkDescriptorSet sets[UBO_BLOCK_COUNT];

    /* For set N, bind a UBO of (N + 1) * UBO_BLOCK_SIZE bytes */
    for (unsigned i = 0; i < UBO_BLOCK_COUNT; ++i) {
        sets[i] = qoAllocateDescriptorSet(t_device,
            .descriptorPool = t_descriptor_pool,
            .pSetLayouts = &set_layout);

        vkUpdateDescriptorSets(t_device, /* writeCount */ 1,
            (VkWriteDescriptorSet[]) {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = (VkDescriptorBufferInfo[]) {
                        {
                            .buffer = buffer,
                            .offset = 0,
                            .range = (i + 1) * UBO_BLOCK_SIZE,
                        }
                    },
                },
            }, 0, NULL);
    }

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .renderPass = t_render_pass,
            .framebuffer = t_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 0.0, 0.0, 0.0, 0.0 } } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    /* Render each descriptor set to a row of the framebuffer */
    for (unsigned i = 0; i < UBO_BLOCK_COUNT; ++i) {
        VkViewport vp = { 0.0f, (float) i, (float) UBO_BLOCK_COUNT, 1.0f, 0.0f, 1.0f };

        vkCmdSetViewport(t_cmd_buffer, /* firstViewport */ 0, /* viewportCount */ 1, &vp);

        vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout, /* firstSet */ 0,
                                /* descriptorSetCount */ 1, &sets[i],
                                /* dynamicOffsetCount */ 0,
                                /* pDynamicOffsets */ NULL);

        vkCmdDraw(t_cmd_buffer, /* vertexCount */ 4, /* instanceCount */ 1,
                  /* firstVertex */ 0, /* firstInstance */ 0);
    }

    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

test_define {
    .name = "func.ubo.robust-push-ubo-full-range",
    .start = test,
    .image_filename = "func.ubo.robust-push-ubo-full-range.ref.png",
    .robust_buffer_access = true,
    .descriptor_count = {
        [VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = 128
    },
    .descriptor_sets = 128
};

