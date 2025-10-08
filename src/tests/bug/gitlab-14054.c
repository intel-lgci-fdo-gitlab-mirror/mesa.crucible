// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-14054-spirv.h"

// \file
// Reproduce an Intel compiler bug from mesa#14054.

static void
test(void)
{
    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        QO_TARGET_ENV spirv1.4

        void main()
        {
        }
    );
    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_TARGET_ENV spirv1.4
        QO_EXTENSION GL_EXT_nonuniform_qualifier : enable
        layout(set=0, binding=0, rgba8) uniform image2D image_array[];

        void main()
        {
            imageStore(image_array[nonuniformEXT(int(gl_FragCoord.x) % 2)],
                       ivec2(0, 0), vec4(0.0));
        }
    );

    VkDescriptorSetLayout set_layout;
    set_layout = qoCreateDescriptorSetLayout(t_device,
        .bindingCount = 1,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 2,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
            },
        });

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout);

    VkPipeline pipeline;
    VkResult result = vkCreateGraphicsPipelines(t_device, t_pipeline_cache, 1,
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
                    {
                        .binding = 0,
                        .stride = 8,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    },
                },
                .vertexAttributeDescriptionCount = 1,
                .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
                    {
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = 0
                    },
                },
            },
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            },
            .pViewportState = &(VkPipelineViewportStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = (VkViewport[]) {
                    {
                        .width = 1, .height = 1, .minDepth = 0.0f, .maxDepth = 1.0f,
                    }
                },
                .scissorCount = 1,
                .pScissors = (VkRect2D[]) {
                    { .extent = { 1, 1 }, },
                },
            },
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            },
            .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            },
            .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            },
            .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            },
            .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            },
            .stageCount = 2,
            .pStages = (VkPipelineShaderStageCreateInfo[]){
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vs,
                    .pName = "main",
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fs,
                    .pName = "main",
                },
            },
            .renderPass = t_render_pass,
            .layout = pipeline_layout,
        }, NULL, &pipeline);
    t_assert(result == VK_SUCCESS);

    vkDestroyPipeline(t_device, pipeline, NULL);

    t_pass();
}

test_define {
    .name = "bug.gitlab-14054",
    .start = test,
    .no_image = true,
};
