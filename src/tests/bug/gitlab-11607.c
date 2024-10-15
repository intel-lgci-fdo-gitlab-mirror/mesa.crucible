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

#include "tapi/t.h"

#include "src/tests/bug/gitlab-11607-spirv.h"

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/11607
 *
 * The issue about flushing copy query results from query pool to buffer when
 * the hw pipeline is in gpgpu mode.
 */

static void
test()
{
    static const uint32_t query_count = 2;

    VkQueryPool query_pool = qoCreateQueryPool(t_device,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = query_count);

    static const uint32_t buffer_size = query_count * sizeof(uint64_t);
    VkBuffer buffer = qoCreateBuffer(t_device,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkDeviceMemory buffer_mem = qoAllocBufferMemory(t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer, buffer_mem, 0);

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device);

    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,

        layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

        void main()
        {
        }
    );

    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
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

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdResetQueryPool(t_cmd_buffer, query_pool, 0, query_count);
    vkCmdWriteTimestamp(t_cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        query_pool, 0);
    vkCmdDispatch(t_cmd_buffer, 1, 1, 1);
    vkCmdWriteTimestamp(t_cmd_buffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        query_pool, 1);

    vkCmdPipelineBarrier(t_cmd_buffer,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 1,
        &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = buffer_size,
        }, 0, NULL);

    // Clear buffer into 0u.
    uint64_t sentinel_value = 0;
    vkCmdFillBuffer(t_cmd_buffer, buffer, 0, buffer_size, sentinel_value);

    vkCmdPipelineBarrier(t_cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 1,
        &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = buffer_size,
        }, 0, NULL);

    vkCmdCopyQueryPoolResults(t_cmd_buffer,
        query_pool, 0, query_count, buffer, 0, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    vkCmdPipelineBarrier(t_cmd_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0, 0, NULL, 1,
        &(VkBufferMemoryBarrier) {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = buffer_size,
        }, 0, NULL);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    uint64_t *data = qoMapMemory(t_device, buffer_mem, 0, VK_WHOLE_SIZE, 0);
    for (unsigned i = 0; i < query_count; i++) {
        t_assertf(data[i] != 0,
            "buffer at %u is zero which is expected to be non-zero.", i);
    }
}

test_define {
    .name = "bug.gitlab-11607",
    .start = test,
    .no_image = true,
};
