// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-12794-spirv.h"

// \file
// Reproduce an Intel compiler bug from mesa#12794.
//
// The HW only deals with SIMD16 messages, with SIMD32 shaders we have
// to lower and our backend screws up the lowering.

struct ssbo_data {
    uint16_t result[32];
};

static void
test(void)
{
    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types : require

        layout(binding = 0, std430) buffer block2 {
            float16_t result[32];
        } ssbo;
        layout(binding = 1) uniform sampler2D smp;

        layout(local_size_x = 32) in;
        void main()
        {
          ssbo.result[gl_LocalInvocationIndex] =
            f16vec4(textureGrad(smp,
                                vec2(float(gl_LocalInvocationIndex) / 32.0, 0),
                                vec2(float(gl_LocalInvocationID.x)),
                                vec2(float(gl_LocalInvocationID.x)))).x;
        }
    );

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(t_device,
        .bindingCount = 2,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
          {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL,
          },
          {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL,
          },
        });

    VkPipelineLayout pipeline_layout =
      qoCreatePipelineLayout(t_device,
                             .setLayoutCount = 1,
                             .pSetLayouts = &set_layout);

    VkPipeline pipeline;
    vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
                             &(VkComputePipelineCreateInfo) {
                               .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                               .stage = {
                                 .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                 .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                                 .module = cs,
                                 .pName = "main",
                               },
                               .flags = 0,
                               .layout = pipeline_layout
                             }, NULL,
                             &pipeline);
    t_cleanup_push_vk_pipeline(t_device, pipeline);

    VkImage image =
      qoCreateImage(t_device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = VK_FORMAT_R32_SFLOAT,
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .extent = {
                      .width = 32,
                      .height = 1,
                      .depth = 1,
                    },
                    .tiling = VK_IMAGE_TILING_LINEAR,
                    .usage = VK_IMAGE_USAGE_SAMPLED_BIT);
    VkDeviceMemory image_mem =
      qoAllocImageMemory(t_device, image,
                         .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    qoBindImageMemory(t_device, image, image_mem, 0);

    VkImageView image_view =
      qoCreateImageView(t_device,
                        .image = image,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = VK_FORMAT_R32_SFLOAT,
                        .subresourceRange = {
                          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .baseMipLevel = 0,
                          .levelCount = 1,
                          .baseArrayLayer = 0,
                          .layerCount = 1,
                        });

    VkSampler sampler =
      qoCreateSampler(t_device,
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

    {
      float *image_map = qoMapMemory(t_device, image_mem,
                                     0, 32 * sizeof(float), 0);
      for (unsigned i = 0; i < 32; i++)
        image_map[i] = (float)i;
    }

    VkBuffer buffer = qoCreateBuffer(t_device,
                                     .size = sizeof(struct ssbo_data),
                                     .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDeviceMemory buffer_mem =
      qoAllocBufferMemory(t_device, buffer,
                          .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer, buffer_mem, 0);

    VkDescriptorSet set = qoAllocateDescriptorSet(t_device,
                                                  .descriptorPool = t_descriptor_pool,
                                                  .pSetLayouts = &set_layout);

    vkUpdateDescriptorSets(t_device,
                           /*writeCount*/ 2,
                           (VkWriteDescriptorSet[]) {
                             {
                               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = set,
                               .dstBinding = 0,
                               .dstArrayElement = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .pBufferInfo = &(VkDescriptorBufferInfo) {
                                 .buffer = buffer,
                                 .offset = 0,
                                 .range = VK_WHOLE_SIZE,
                               },
                             },
                             {
                               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = set,
                               .dstBinding = 1,
                               .dstArrayElement = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               .pImageInfo = &(VkDescriptorImageInfo) {
                                 .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                                 .imageView = image_view,
                                 .sampler = sampler,
                               },
                             },
                           }, 0, NULL);

    vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                         0, NULL,
                         1,
                         &(VkBufferMemoryBarrier) {
                           .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                           .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                           .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                           .buffer = buffer,
                           .offset = 0,
                           .size = VK_WHOLE_SIZE,
                         },
                         1,
                         &(VkImageMemoryBarrier) {
                           .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                           .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                           .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                           .image = image,
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                           .image = image,
                           .subresourceRange = {
                             .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1,
                           },
                         });

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout, 0, 1, &set, 0, NULL);

    vkCmdDispatch(t_cmd_buffer, 1, 1, 1);

    vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT, 0,
                         0, NULL,
                         1,
                         &(VkBufferMemoryBarrier) {
                           .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                           .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                           .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                           .buffer = buffer,
                           .offset = 0,
                           .size = VK_WHOLE_SIZE,
                         }, 0, NULL);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);


    struct ssbo_data *data = qoMapMemory(t_device, buffer_mem,
                                         0, sizeof(*data), 0);

    const uint16_t expected_values[32] = {
      0x0000, 0x3800, 0x3e00, 0x4100,
      0x4300, 0x4480, 0x4580, 0x4680,
      0x4780, 0x4840, 0x48c0, 0x4940,
      0x49c0, 0x4a40, 0x4ac0, 0x4b40,
      0x4bc0, 0x4c20, 0x4c60, 0x4ca0,
      0x4ce0, 0x4d20, 0x4d60, 0x4da0,
      0x4de0, 0x4e20, 0x4e60, 0x4ea0,
      0x4ee0, 0x4f20, 0x4f60, 0x4fa0
    };
    for (unsigned i = 0; i < 32; i++)
      t_assert(data->result[i] == expected_values[i]);

    t_pass();
}

test_define {
    .name = "bug.gitlab-12794",
    .start = test,
    .no_image = true,
};
