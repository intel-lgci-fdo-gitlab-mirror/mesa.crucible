// Copyright 2023 Intel Corporation
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

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/9013
 *
 * The issue is an unsynchronized write from compute shader to a buffer then
 * written with query copy results.
 */

#include "tapi/t.h"

#include "src/tests/bug/gitlab-9013-spirv.h"

#define DATA_SIZE (8 * 4)

static void
test(void)
{
    VkResult result;

    VkDescriptorSetLayout compute_set_layout;
    compute_set_layout = qoCreateDescriptorSetLayout(t_device,
        .bindingCount = 1,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
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
        layout(set = 0, binding = 0, std430) buffer block2 {
            uint data[];
        } ssbo;

        layout (local_size_x = 1) in;
        void main()
        {
            ssbo.data[0] = 0;
            ssbo.data[1] = 1;
            ssbo.data[2] = 2;
            ssbo.data[3] = 3;
            ssbo.data[4] = 4;
            ssbo.data[5] = 5;
            ssbo.data[6] = 6;
            ssbo.data[7] = 7;
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

    VkBuffer ssbo = qoCreateBuffer(t_device,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .size = DATA_SIZE);
    VkDeviceMemory ssbo_mem = qoAllocBufferMemory(t_device, ssbo,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, ssbo, ssbo_mem, 0);

    VkDescriptorSet set;
    result = vkAllocateDescriptorSets(t_device,
        &(VkDescriptorSetAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = t_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = (VkDescriptorSetLayout[]) {
                compute_set_layout,
            },
        }, &set);
    t_assert(result == VK_SUCCESS);

    vkUpdateDescriptorSets(t_device,
        1, /* writeCount */
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 0,
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

    VkQueryPool pool = qoCreateQueryPool(t_device,
                                         .queryType = VK_QUERY_TYPE_TIMESTAMP,
                                         .queryCount = 4);

    vkCmdResetQueryPool(t_cmd_buffer, pool, 0, 4);

    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, 0);
    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, 1);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      compute_pipeline);
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            compute_pipeline_layout, 0, 1,
                            &set, 0, NULL);

    vkCmdDispatch(t_cmd_buffer, 1, 1, 1);

    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, 2);
    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, 3);

    vkCmdPipelineBarrier(t_cmd_buffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, NULL, 1,
                         (VkBufferMemoryBarrier[]) {
                             {
                                 .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                 .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                                 .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                 .buffer = ssbo,
                                 .offset = 0,
                                 .size = VK_WHOLE_SIZE,
                             },
                         }, 0, NULL);

    vkCmdCopyQueryPoolResults(t_cmd_buffer, pool, 0, 4,
                              ssbo, 16, 4, VK_QUERY_RESULT_WAIT_BIT);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    unsigned *ssbo_data = qoMapMemory(t_device, ssbo_mem, 0, VK_WHOLE_SIZE, 0);
    for (unsigned i = 0; i < 4; i++) {
        t_assertf(ssbo_data[i] == i,
                  "buffer mismatch at %u: found 0x%08x, expected 0x%08x",
                  i, ssbo_data[i], i);
    }
    for (unsigned i = 4; i < 8; i++) {
        t_assertf(ssbo_data[i] != i,
                  "buffer mismatch at %u: found 0x%08x, expected not 0x%08x",
                  i, ssbo_data[i], i);
    }

    vkDestroyPipeline(t_device, compute_pipeline, NULL);
}

test_define {
    .name = "bug.gitlab.9013",
    .start = test,
    .no_image = true,
};
