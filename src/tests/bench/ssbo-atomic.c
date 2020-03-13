// Copyright 2015 Intel Corporation
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

#include <inttypes.h>
#include <math.h>

#include "tapi/t.h"

#include "src/tests/bench/ssbo-atomic-spirv.h"

#define NUM_ITERATIONS 10

static void
test_atomics(bool host_coherent)
{
    const uint64_t num_atomics_per_workgroup = 16 * 1024ull * 128;
    VkShaderModule cs = qoCreateShaderModuleGLSL(
        t_device, COMPUTE,

        layout(local_size_x = 128) in;

        layout(set = 0, binding = 0) buffer block {
            uint a[16];
        } ssbo;

        void main()
        {
            for (uint i = 0; i < 1024; i++) {
                atomicAdd(ssbo.a[0], 0);
                atomicAdd(ssbo.a[1], 1);
                atomicAdd(ssbo.a[2], 2);
                atomicAdd(ssbo.a[3], 3);
                atomicAdd(ssbo.a[4], 4);
                atomicAdd(ssbo.a[5], 5);
                atomicAdd(ssbo.a[6], 6);
                atomicAdd(ssbo.a[7], 7);
                atomicAdd(ssbo.a[8], 8);
                atomicAdd(ssbo.a[9], 9);
                atomicAdd(ssbo.a[10], 10);
                atomicAdd(ssbo.a[11], 11);
                atomicAdd(ssbo.a[12], 12);
                atomicAdd(ssbo.a[13], 13);
                atomicAdd(ssbo.a[14], 14);
                atomicAdd(ssbo.a[15], 15);
            }
        }
    );

    VkBuffer atomic_buffer = qoCreateBuffer(t_device,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .size = 64);
    VkDeviceMemory atomic_memory = qoAllocBufferMemory(t_device, atomic_buffer,
        .properties = host_coherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0);
    qoBindBufferMemory(t_device, atomic_buffer, atomic_memory, 0);

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(t_device,
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

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout);

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
            .layout = pipeline_layout,
        }, NULL, &pipeline);
    t_assert(result == VK_SUCCESS);
    t_cleanup_push_vk_pipeline(t_device, pipeline);

    VkDescriptorSet set =
        qoAllocateDescriptorSet(t_device,
                                .descriptorPool = t_descriptor_pool,
                                .pSetLayouts = &set_layout);
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
                .pBufferInfo = (VkDescriptorBufferInfo[]) {
                    {
                        .buffer = atomic_buffer,
                        .offset = 0,
                        .range = VK_WHOLE_SIZE,
                    }
                },
            },
        }, 0, NULL);

    VkQueryPool query = qoCreateQueryPool(t_device,
                                          .queryType = VK_QUERY_TYPE_TIMESTAMP,
                                          .queryCount = 2);


    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout, 0, 1, &set, 0, NULL);
    vkCmdResetQueryPool(t_cmd_buffer, query, 0, 2);
    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        query, 0);
    const uint64_t num_workgroups = 10;
    vkCmdDispatch(t_cmd_buffer, num_workgroups, 1, 1);
    vkCmdWriteTimestamp(t_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        query, 1);
    qoEndCommandBuffer(t_cmd_buffer);

    double aps[NUM_ITERATIONS];
    for (unsigned i = 0; i < NUM_ITERATIONS; i++) {
        qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
        qoQueueWaitIdle(t_queue);

        uint64_t query_results[2];
        vkGetQueryPoolResults(t_device, query, 0, 2,
                              sizeof(query_results), query_results,
                              sizeof(query_results[0]),
                              VK_QUERY_RESULT_64_BIT);

        uint64_t time = query_results[1] - query_results[0];
        double seconds =
            (time * (double)t_physical_dev_props->limits.timestampPeriod) /
            1000000000.0;

        uint64_t num_atomics = num_atomics_per_workgroup * num_workgroups;
        aps[i] = num_atomics / seconds;
        int aps_exp = log10(aps[i]);
        double aps_mantissa = aps[i] * pow(10, -aps_exp);

        logi("Iteration %u: %0.4fx10^%u atomics per second",
             i, aps_mantissa, aps_exp);
    }

    double mean = 0;
    for (unsigned i = 0; i < NUM_ITERATIONS; i++)
        mean += aps[i];
    mean /= NUM_ITERATIONS;

    double var = 0;
    for (unsigned i = 0; i < NUM_ITERATIONS; i++)
        var += (aps[i] - mean) * (aps[i] - mean);

    double stddev = sqrt(var);

    int mean_exp = log10(mean);
    double mean_mantissa = mean * pow(10, -mean_exp);
    int stddev_exp = log10(stddev);
    double stddev_mantissa = stddev * pow(10, -stddev_exp);

    logi("Mean: %0.4fx10^%u +/- %0.4fx10^%u atomics per second",
         mean_mantissa, mean_exp, stddev_mantissa, stddev_exp);
}

static void
test_host_coherent_atomics()
{
    test_atomics(true);
}

test_define {
    .name = "bench.atomic.host-coherent",
    .start = test_host_coherent_atomics,
    .no_image = true,
};

static void
test_non_host_coherent_atomics()
{
    test_atomics(false);
}

test_define {
    .name = "bench.atomic.non-host-coherent",
    .start = test_non_host_coherent_atomics,
    .no_image = true,
};
