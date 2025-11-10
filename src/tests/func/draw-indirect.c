// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"

#include "src/tests/func/draw-indirect-spirv.h"

struct params {
    const void *indirect_data;
    size_t indirect_data_size;
    uint32_t indirect_data_stride;
};

static VkBuffer
create_buffer(VkBufferUsageFlags usage,
              VkDeviceSize size,
              const void *data)
{
    VkBuffer buffer =
        qoCreateBuffer(t_device, .size = size, .usage = usage);

    VkDeviceMemory mem = qoAllocBufferMemory(
        t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *map = qoMapMemory(t_device, mem, 0, size, 0);
    qoBindBufferMemory(t_device, buffer, mem, 0);

    memcpy(map, data, size);

    return buffer;
}

static void
test(void)
{
    const struct params *params = t_user_data;

    VkPipeline pipeline;
    VkBuffer indirect_buffer = create_buffer(
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        params->indirect_data_size,
        params->indirect_data);

    const float vertices_data[] = {
         0.0f, -1.0f,
        -1.0f, -1.0f,
         0.0f,  0.0f,
        -1.0f,  0.0f,

         1.0f, -1.0f,
         0.0f, -1.0f,
         1.0f,  0.0f,
         0.0f,  0.0f,

         0.0f,  0.0f,
        -1.0f,  0.0f,
         0.0f,  1.0f,
        -1.0f,  1.0f,

         1.0f,  0.0f,
         0.0f,  0.0f,
         1.0f,  1.0f,
         0.0f,  1.0f,
    };
    VkBuffer vertices = create_buffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(vertices_data),
        vertices_data);

    const float colors_data[] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
    };
    VkBuffer colors = create_buffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(colors_data),
        colors_data);

    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        QO_VERSION 460

        layout(location = 0) in vec4 a_position;
        layout(location = 1) in vec4 a_color;
        layout(location = 0) out vec4 v_color;

        void main()
        {
            gl_Position = a_position;
            v_color = a_color;
        }
    );

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device);

    pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .vertexShader = vs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
                QO_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO_DEFAULTS,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            },
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 2,
                .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
                    {
                        .binding = 0,
                        .stride = 8,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    },
                    {
                        .binding = 1,
                        .stride = 16,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    }
                },
                .vertexAttributeDescriptionCount = 2,
                .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
                    {
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = 0
                    },
                    {
                        .location = 1,
                        .binding = 1,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = 0
                    }
                },
            },
            .layout = pipeline_layout,
            .renderPass = t_render_pass,
            .subpass = 0,
        }});

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = t_render_pass,
            .framebuffer = t_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = {0.0f, 0.0f, 0.0f, 0.0f} } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 2,
                           (VkBuffer[]) { vertices, colors },
                           (VkDeviceSize[]) { 0, 0 });

    vkCmdDrawIndirect(t_cmd_buffer, indirect_buffer,
                      0, 4, params->indirect_data_stride);

    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

static const uint32_t indirect_data_aligned[] = {
    // indirect 0
    4,
    1,
    0,
    0,
    // indirect 1
    4,
    1,
    4,
    0,
    // indirect 2
    4,
    1,
    8,
    0,
    // indirect 3
    4,
    1,
    12,
    0,
};

test_define {
    .name = "func.draw-indirect-aligned",
    .start = test,
    .user_data = &(struct params) {
        .indirect_data = indirect_data_aligned,
        .indirect_data_size = sizeof(indirect_data_aligned),
        .indirect_data_stride = sizeof(VkDrawIndirectCommand),
    },
};

static const uint32_t indirect_data_unaligned[] = {
    // indirect 0
    4,
    1,
    0,
    0,
    0xdeadead, // padding
    // indirect 1
    4,
    1,
    4,
    0,
    0xdeadead, // padding
    // indirect 2
    4,
    1,
    8,
    0,
    0xdeadead, // padding
    // indirect 2
    4,
    1,
    12,
    0,
    0xdeadead, // padding
};

test_define {
    .name = "func.draw-indirect-unaligned",
    .start = test,
    .user_data = &(struct params) {
        .indirect_data = indirect_data_unaligned,
        .indirect_data_size = sizeof(indirect_data_unaligned),
        .indirect_data_stride = sizeof(VkDrawIndirectCommand) + 4,
    },
};
