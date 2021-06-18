// Copyright 2021 Intel Corporation
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

#define NUM_ITERATIONS (10)

// Use a semaphore both for waiting and signaling.
static void
test_sync_wait_signal_wait(void)
{
    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
    } buffers[NUM_ITERATIONS];
    VkDeviceSize buffer_size = 1 * 1024 * 1024;

    for (int i = 0; i < ARRAY_LENGTH(buffers); i++) {
        buffers[i].buf = qoCreateBuffer(t_device, .size = buffer_size,
                                        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        VkMemoryRequirements buffer_reqs =
            qoGetBufferMemoryRequirements(t_device, buffers[i].buf);

        buffers[i].mem =
            qoAllocMemoryFromRequirements(t_device, &buffer_reqs,
                                          .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        qoBindBufferMemory(t_device, buffers[i].buf, buffers[i].mem, 0);
    }

    VkCommandBuffer cmd_buffers[NUM_ITERATIONS - 1];

    for (int i = 0; i < ARRAY_LENGTH(buffers) - 1; i++) {
        cmd_buffers[i] = qoAllocateCommandBuffer(t_device, t_cmd_pool);

        qoBeginCommandBuffer(cmd_buffers[i]);

        vkCmdCopyBuffer(cmd_buffers[i], buffers[i].buf, buffers[i + 1].buf, 1,
                        &(VkBufferCopy) {
                            .srcOffset = 0,
                            .dstOffset = 0,
                            .size = buffer_size,
                        });

        qoEndCommandBuffer(cmd_buffers[i]);
    }

    uint32_t *first_map = qoMapMemory(t_device, buffers[0].mem, 0, buffer_size, 0);
    uint32_t *last_map = qoMapMemory(t_device, buffers[ARRAY_LENGTH(buffers) - 1].mem, 0, buffer_size, 0);
    memset(first_map, 0x33, buffer_size);
    vkFlushMappedMemoryRanges(t_device, 1,
                              &(VkMappedMemoryRange) {
                                  .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                  .memory = buffers[0].mem,
                                  .offset = 0,
                                  .size = buffer_size,
                              });

    VkSemaphore sem;
    VkResult result = vkCreateSemaphore(t_device,
                                        &(VkSemaphoreCreateInfo) {
                                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                        }, NULL, &sem);
    t_assert(result == VK_SUCCESS);

    for (int i = 0; i < ARRAY_LENGTH(buffers) - 1; i++) {
        VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkSubmitInfo submit = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffers[i],
            .waitSemaphoreCount = i == 0 ? 0 : 1,
            .pWaitSemaphores = &sem,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &sem,
            .pWaitDstStageMask = &flags,
        };

        result = vkQueueSubmit(t_queue, 1, &submit, VK_NULL_HANDLE);
        t_assert(result == VK_SUCCESS);

        vkQueueWaitIdle(t_queue);
    }

    vkQueueWaitIdle(t_queue);

    vkDestroySemaphore(t_device, sem, NULL);

    vkInvalidateMappedMemoryRanges(t_device, 2,
                                   (VkMappedMemoryRange[]) {
                                       {
                                           .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                           .memory = buffers[0].mem,
                                           .offset = 0,
                                           .size = buffer_size,
                                       },
                                       {
                                           .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                           .memory = buffers[ARRAY_LENGTH(buffers) - 1].mem,
                                           .offset = 0,
                                           .size = buffer_size,
                                       },
                                   });

    for (int i = 0; i < buffer_size / 4; i++) {
        assert(first_map[i] == last_map[i]);
        assert(first_map[i] == 0x33333333);
    }
}

test_define {
    .name = "func.sync.semaphore.wait-signal-wait",
    .start = test_sync_wait_signal_wait,
    .no_image = true,
};
