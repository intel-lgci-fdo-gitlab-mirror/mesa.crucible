// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"

#include "src/tests/bug/gitlab-16764-spirv.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/16764
 *
 * A first fragment shader writes a color into a buffer through an SSBO, then
 * a second fragment shader reads it through a UBO after a
 * FRAGMENT_SHADER/SHADER_WRITE -> FRAGMENT_SHADER/UNIFORM_READ barrier. The
 * second shader reads stale data when the UBO is promoted to push constants.
 */

static void
test(void)
{
    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        void main()
        {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
            gl_PointSize = 64.0;
        }
    );

    /* Writes the UBO data for fs2 */
    VkShaderModule fs1 = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(set = 0, binding = 1) buffer block {
            vec4 color;
        };

        void main()
        {
            color = vec4(0.0, 1.0, 0.0, 1.0);
        }
    );

    VkShaderModule fs2 = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;
        layout(set = 0, binding = 0) uniform block {
            vec4 color;
        };

        void main()
        {
            f_color = color;
        }
    );

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(
        t_device,
        .bindingCount = 2,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
            },
        });

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(
        t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout);

    /* First render pass, no attachments, fs1 only writes to the buffer. */
    VkRenderPass write_pass = qoCreateRenderPass(t_device,
        .attachmentCount = 0,
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                QO_SUBPASS_DESCRIPTION_DEFAULTS,
            },
        });

    VkFramebuffer write_framebuffer = qoCreateFramebuffer(t_device,
        .renderPass = write_pass,
        .width = t_width,
        .height = t_height,
        .layers = 1,
        .attachmentCount = 0);

    /* Compatible with t_render_pass, but the attachments are already in the
     * right layout. With an UNDEFINED initial layout, the render pass emits
     * its own transition barriers and the bug doesn't show up.
     */
    VkRenderPass render_pass = qoCreateRenderPass(t_device,
        .attachmentCount = 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                QO_ATTACHMENT_DESCRIPTION_DEFAULTS,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    },
                },
            },
        });

    VkFramebuffer framebuffer = qoCreateFramebuffer(t_device,
        .renderPass = render_pass,
        .width = t_width,
        .height = t_height,
        .layers = 1,
        .attachmentCount = 1,
        .pAttachments = (VkImageView[]) {
            t_color_image_view,
        });

    VkPipelineVertexInputStateCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipeline write_pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
            .vertexShader = vs,
            .fragmentShader = fs1,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &vi_create_info,
            .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .attachmentCount = 0,
            },
            .flags = 0,
            .layout = pipeline_layout,
            .renderPass = write_pass,
            .subpass = 0,
        }});

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
            .vertexShader = vs,
            .fragmentShader = fs2,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &vi_create_info,
            .flags = 0,
            .layout = pipeline_layout,
            .renderPass = render_pass,
            .subpass = 0,
        }});

    VkBuffer ubo = qoCreateBuffer(t_device,
        .size = 4 * sizeof(float),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    VkDeviceMemory ubo_mem = qoAllocBufferMemory(t_device, ubo,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    qoBindBufferMemory(t_device, ubo, ubo_mem, /*offset*/ 0);

    VkDescriptorSet set = qoAllocateDescriptorSet(t_device,
        .descriptorPool = t_descriptor_pool,
        .pSetLayouts = &set_layout);

    VkDescriptorBufferInfo ubo_info = {
        .buffer = ubo,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    vkUpdateDescriptorSets(t_device, 2,
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &ubo_info,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &ubo_info,
            },
        }, 0, NULL);

    vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, 0, 1, &set, 0, NULL);

    /* fs1 writes the UBO data */
    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = write_pass,
            .framebuffer = write_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      write_pipeline);
    vkCmdDraw(t_cmd_buffer, /*vertexCount*/ 1, /*instanceCount*/ 1,
              /*firstVertex*/ 0, /*firstInstance*/ 0);
    vkCmdEndRenderPass(t_cmd_buffer);

    /* The layout transitions have to be part of the same barrier as the UBO
     * memory barrier, the bug doesn't show up otherwise.
     */
    vkCmdPipelineBarrier(
        t_cmd_buffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        1, &(VkMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
        },
        0, NULL,
        1, (VkImageMemoryBarrier[]) {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = t_color_image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            },
        });

    /* fs2 reads the UBO data */
    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 2,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 1.0, 0.0, 0.0, 1.0 } } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(t_cmd_buffer, /*vertexCount*/ 1, /*instanceCount*/ 1,
              /*firstVertex*/ 0, /*firstInstance*/ 0);
    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

test_define {
    .name = "bug.gitlab-16764",
    .start = test,
    .image_filename = "32x32-green.ref.png",
};
