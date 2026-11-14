// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"

#include "src/tests/bug/gitlab-16722-spirv.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/16722
 *
 * Improper PIPE_CONTROL stall bit for fragment to compute dependency.
 */

#define GET_DEVICE_FUNCTION_PTR(name) \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(t_device, #name); \
    t_assert(name != NULL);

static void
test()
{
    t_require_ext("VK_KHR_dynamic_rendering");

    VkShaderModule cs = qoCreateShaderModuleGLSL(
        t_device, COMPUTE,

        layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
        layout(set = 0, binding = 0, rgba8) uniform writeonly image2D img;

        void main()
        {
            ivec2 id = ivec2(gl_GlobalInvocationID.xy);
            imageStore(img, id, vec4(1.0, 1.0, 1.0, 1.0));
        }
        );

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(
        t_device,
        .bindingCount = 1,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
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
    VkResult result = vkCreateComputePipelines(
        t_device, t_pipeline_cache, 1,
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
            .layout = pipeline_layout
        }, NULL, &pipeline);
    t_assert(result == VK_SUCCESS);
    t_cleanup_push_vk_pipeline(t_device, pipeline);

    VkImage image = qoCreateImage(t_device,
                                  .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
                                  .imageType = VK_IMAGE_TYPE_2D,
                                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                                  .extent = {64, 64, 1},
                                  .mipLevels = 1,
                                  .arrayLayers = 1,
                                  .samples = VK_SAMPLE_COUNT_1_BIT,
                                  .tiling = VK_IMAGE_TILING_OPTIMAL,
                                  .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                  VK_IMAGE_USAGE_STORAGE_BIT,
                                  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    VkDeviceMemory image_mem = qoAllocImageMemory(
        t_device, image, .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    qoBindImageMemory(t_device, image, image_mem, 0);
    VkImageView view = qoCreateImageView(t_device,
                                         .image = image,
                                         .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                         .format = VK_FORMAT_R8G8B8A8_UNORM);

    VkBuffer buffer = qoCreateBuffer(
        t_device,
        .size = 64 * 64 * 4,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkDeviceMemory buffer_mem = qoAllocBufferMemory(
        t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer, buffer_mem, 0);

    VkDescriptorSet set = qoAllocateDescriptorSet(t_device,
                                                  .descriptorPool = t_descriptor_pool,
                                                  .pSetLayouts = &set_layout);

    vkUpdateDescriptorSets(t_device, 1,
                           (VkWriteDescriptorSet[]) {
                               {
                                   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   .dstSet = set,
                                   .dstBinding = 0,
                                   .dstArrayElement = 0,
                                   .descriptorCount = 1,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   .pImageInfo = &(VkDescriptorImageInfo) {
                                       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                                       .imageView = view,
                                   },
                               },
                           }, 0, NULL);

    /* Beginning of commands */

    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1,
        &(VkImageMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        });

    {
        vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipeline_layout, 0, 1, &set, 0, NULL);
        vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        vkCmdDispatch(t_cmd_buffer, 64, 64, 1);
    }

    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, NULL, 0, NULL, 1,
        &(VkImageMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        });

    {
        GET_DEVICE_FUNCTION_PTR(vkCmdBeginRenderingKHR);
        GET_DEVICE_FUNCTION_PTR(vkCmdEndRenderingKHR);

        VkRenderingAttachmentInfo att = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = { .float32 = { 0.0, 0.0, 0.0, 0.0, }}},
        };
        VkRenderingInfo ri = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {64, 64}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &att};
        vkCmdBeginRenderingKHR(t_cmd_buffer, &ri);
        vkCmdEndRenderingKHR(t_cmd_buffer);
    }

    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1,
        &(VkImageMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        });

    {
        vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipeline_layout, 0, 1, &set, 0, NULL);
        vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        vkCmdDispatch(t_cmd_buffer, 64, 64, 1);
    }

    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL,
        1, &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .buffer = buffer,
            .size = VK_WHOLE_SIZE,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        },
        1, &(VkImageMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        });

    {
        vkCmdCopyImageToBuffer(
            t_cmd_buffer,
            image,
            VK_IMAGE_LAYOUT_GENERAL,
            buffer,
            1, &(VkBufferImageCopy) {
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                },
                .imageExtent = {
                    .width = 64,
                    .height = 64,
                    .depth = 1,
                },
            });
    }

    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0, 0, NULL,
        1, &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .buffer = buffer,
            .size = VK_WHOLE_SIZE,
        }, 0, NULL);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    uint32_t *data = qoMapMemory(t_device, buffer_mem, 0, VK_WHOLE_SIZE, 0);
    for (unsigned i = 0; i < 64 * 64; i++) {
        t_assertf(data[i] == 0xffffffff,
                  "buffer at dword %i, unexpected color 0x%08x.", i, data[i]);
    }
}

test_define {
    .name = "bug.gitlab-16722",
    .features = {
        .dynamicRendering = true,
    },
    .start = test,
    .no_image = true,
};
