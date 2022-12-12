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

#include "tapi/t.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/5711
 *
 * The issue is that we're not applying robustness for layered image
 * with image that have a single layer. This is due to performance
 * workaround on Intel Gfx12+ HW.
 */
#include "src/tests/bug/gitlab-5711-spirv.h"

static void
test_gitlab_5711(void)
{
  t_require_ext("VK_EXT_robustness2");

  VkPhysicalDeviceRobustness2FeaturesEXT robustness2_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
  };
  VkPhysicalDeviceFeatures2 features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &robustness2_features,
  };
  vkGetPhysicalDeviceFeatures2(t_physical_dev, &features);

  if (!robustness2_features.robustImageAccess2)
      t_skipf("robustImageAccess2 not available");

  VkResult result;

  VkDescriptorSetLayout compute_set_layout;
  compute_set_layout = qoCreateDescriptorSetLayout(t_device,
    .bindingCount = 2,
    .pBindings = (VkDescriptorSetLayoutBinding[]) {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = NULL,
      },
      {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = NULL,
      },
    });

  VkPipelineLayout compute_pipeline_layout = qoCreatePipelineLayout(t_device,
    .setLayoutCount = 1,
    .pSetLayouts = &compute_set_layout);

  VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
    layout(set = 0, binding = 0, r32ui) uniform uimage2DArray image;
    layout(set = 0, binding = 1, std430) buffer block {
      uint data[];
    } ssbo;

    layout (local_size_x = 1) in;
    void main()
    {
      ssbo.data[0] = imageLoad(image, ivec3(0, 0, 1)).x;
    }
  );

  VkPipeline compute_pipeline;
  result = vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
    &(VkComputePipelineCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = cs,
        .pName = "main",
      },
      .flags = 0,
      .layout = compute_pipeline_layout,
    }, NULL, &compute_pipeline);
  t_assert(result == VK_SUCCESS);

  VkDescriptorSet set;
  result =
    vkAllocateDescriptorSets(t_device,
                             &(VkDescriptorSetAllocateInfo) {
                               .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                               .descriptorPool = t_descriptor_pool,
                               .descriptorSetCount = 1,
                               .pSetLayouts = (VkDescriptorSetLayout[]) {
                                 compute_set_layout,
                               },
                             }, &set);
  t_assert(result == VK_SUCCESS);

  VkImage texture =
    qoCreateImage(t_device,
                  .format = VK_FORMAT_R32_UINT,
                  .tiling = VK_IMAGE_TILING_LINEAR,
                  .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                  .extent = {
                    .width = 4,
                    .height = 4,
                    .depth = 1,
                  });
  VkMemoryRequirements texture_reqs =
    qoGetImageMemoryRequirements(t_device, texture);
  VkDeviceMemory texture_mem = qoAllocMemoryFromRequirements(t_device,
    &texture_reqs, .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  qoBindImageMemory(t_device, texture, texture_mem, /*offset*/ 0);

  VkImageView tex_view =
    qoCreateImageView(t_device,
                      .image = texture,
                      .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                      .format = VK_FORMAT_R32_UINT,
                      .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                      });

  VkBuffer ssbo = qoCreateBuffer(t_device,
                                 .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 .size = sizeof(uint32_t));
  VkDeviceMemory ssbo_mem =
    qoAllocBufferMemory(t_device, ssbo,
                        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  qoBindBufferMemory(t_device, ssbo, ssbo_mem, 0);

  vkUpdateDescriptorSets(t_device,
                         2, /* writeCount */
                         (VkWriteDescriptorSet[]) {
                           {
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .dstSet = set,
                             .dstBinding = 0,
                             .dstArrayElement = 0,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                             .pImageInfo = &(VkDescriptorImageInfo) {
                               .imageView = tex_view,
                               .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                             },
                           },
                           {
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .dstSet = set,
                             .dstBinding = 1,
                             .dstArrayElement = 0,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                             .pBufferInfo = &(VkDescriptorBufferInfo) {
                               .buffer = ssbo,
                               .offset = 0,
                               .range = VK_WHOLE_SIZE,
                             },
                           },
                         }, 0, NULL);

  vkCmdPipelineBarrier(t_cmd_buffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 0, NULL, 0, NULL,
                       1, &(VkImageMemoryBarrier) {
                         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                         .srcAccessMask = 0,
                         .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                         .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                         .image = texture,
                         .subresourceRange = {
                           .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1,
                         },
                       });

  vkCmdClearColorImage(t_cmd_buffer,
                       texture,
                       VK_IMAGE_LAYOUT_GENERAL,
                       &(VkClearColorValue) {
                         .float32 = { 1.0, 0.0, 0.0, 0.0 }, },
                       1,
                       &(VkImageSubresourceRange) {
                         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                         .baseMipLevel = 0,
                         .levelCount = 1,
                         .baseArrayLayer = 0,
                         .layerCount = 1,
                       });

  vkCmdPipelineBarrier(t_cmd_buffer,
                       VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 0, NULL,
                       1, &(VkBufferMemoryBarrier) {
                         .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                         .srcAccessMask = 0,
                         .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                         .buffer = ssbo,
                         .offset = 0,
                         .size = VK_WHOLE_SIZE,
                       },
                       0, NULL);

  vkCmdPipelineBarrier(t_cmd_buffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 0, NULL, 0, NULL,
                       1, &(VkImageMemoryBarrier) {
                         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                         .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                         .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                         .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                         .image = texture,
                         .subresourceRange = {
                           .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1,
                         },
                       });

  vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    compute_pipeline);
  vkCmdBindDescriptorSets(t_cmd_buffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          compute_pipeline_layout, 0,
                          1, &set, 0, NULL);

  vkCmdDispatch(t_cmd_buffer, 1, 1, 1);

  vkCmdPipelineBarrier(t_cmd_buffer,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT,
                       0, 0, NULL,
                       1, &(VkBufferMemoryBarrier) {
                         .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                         .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                         .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                         .buffer = ssbo,
                         .offset = 0,
                         .size = VK_WHOLE_SIZE,
                       },
                       0, NULL);

  qoEndCommandBuffer(t_cmd_buffer);

  uint32_t *ssbo_data = qoMapMemory(t_device, ssbo_mem, 0, VK_WHOLE_SIZE, 0);
  *ssbo_data = 0;

  vkFlushMappedMemoryRanges(t_device,
                            1, &(VkMappedMemoryRange) {
                              .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                              .memory = ssbo_mem,
                              .offset = 0,
                              .size = 4,
                            });

  qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
  qoQueueWaitIdle(t_queue);

  t_assertf(*ssbo_data == 0, "buffer mismatch found 0x%08x expected 0,0,0,0",
            *ssbo_data);

  vkDestroyPipeline(t_device, compute_pipeline, NULL);
}

test_define {
  .name = "bug.gitlab-5711",
  .start = test_gitlab_5711,
  .no_image = true,
  .robust_image_access = true,
};
