// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"
#include "util/misc.h"

#include "src/tests/func/issue-11617-spirv.h"

// \file
// Reproduce an Intel compiler bug from mesa#11617.
//
// After a texture is sampled, the read values are stored into registers using
// a LOAD_PAYLOAD pseudo-op. Starting with mesa!30447, values that are only
// used as half-float are sampled as half-float. When all of the components
// read from the texture are used, all is well.
//
// However, components that are never read are not copied using the
// LOAD_PAYLOAD. Instead, UNDEF values are used. In the bug, the UNDEF values
// have type UD. The causes compiler validation to think too much data is
// written. This is not real because the values are not read, so dead code
// elimination will delete the writes.
//
// In all of the CTS, piglit, and crucible, nothing reproduced the case. It
// was only discovered by a compute shader in Q2RTX.

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
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_float16: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require
        layout(location = 0) out vec4 f_color;
        layout(location = 0) in vec4 v_color;
        layout(set = 0, binding = 0) uniform sampler2D tex;

        void main()
        {
            f16vec4 x = f16vec4(texture(tex, vec2(0.1, 0.1)));

            // The important part of this test case is that some of the
            // components returned by texture() command are not used.
            f_color = vec4(uintBitsToFloat(uint(float16BitsToUint16(x.x + x.y)) << 16 | uint(float16BitsToUint16(x.z))), v_color.yzw);
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
    VkDescriptorSetLayout set_layout[2];

    set_layout[0] = qoCreateDescriptorSetLayout(t_device,
            .bindingCount = 1,
            .pBindings = (VkDescriptorSetLayoutBinding[]) {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = NULL,
                },
            });

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = set_layout);

    VkPipeline pipeline = create_pipeline(t_device, pipeline_layout);

    VkDescriptorSet set[1];
    VkResult result = vkAllocateDescriptorSets(t_device,
        &(VkDescriptorSetAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = t_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = set_layout,
        }, set);
    t_assert(result == VK_SUCCESS);

    static const float vertex_data[] = {
        // Triangle coordinates
        -0.5, -0.5, 0.0, 1.0,
         0.5, -0.5, 0.0, 1.0,
         0.0,  0.5, 0.0, 1.0,

         // Color
         1.0,  0.0, 0.0, 0.2,
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

    const uint32_t texture_width = 16, texture_height = 16;

    VkImage texture = qoCreateImage(t_device,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .extent = {
            .width = texture_width,
            .height = texture_height,
            .depth = 1,
        });

    VkMemoryRequirements texture_reqs =
        qoGetImageMemoryRequirements(t_device, texture);

    VkDeviceMemory texture_mem = qoAllocMemoryFromRequirements(t_device,
        &texture_reqs, .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    qoBindImageMemory(t_device, texture, texture_mem, /*offset*/ 0);

    // Initialize texture data
    memset(qoMapMemory(t_device, texture_mem, /*offset*/ 0,
                       texture_reqs.size, /*flags*/ 0),
           192, texture_reqs.size);

    VkImageView tex_view = qoCreateImageView(t_device,
        .image = texture,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM);

    VkSampler sampler = qoCreateSampler(t_device,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0,
        .maxAnisotropy = 0,
        .compareOp = VK_COMPARE_OP_GREATER,
        .minLod = 0,
        .maxLod = 0,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);

    vkUpdateDescriptorSets(t_device,
        1, /* writeCount */
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set[0],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = (VkDescriptorImageInfo[]) {
                    {
                        .imageView = tex_view,
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .sampler = sampler,
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
                            &set[0], 0, NULL);
    vkCmdDraw(t_cmd_buffer, /*vertexCount*/ 3, /*instanceCount*/ 1,
              /*firstVertex*/ 0, /*firstInstance*/ 0);
    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

test_define {
    .name = "func.issue-11617",
    .start = test,
};
