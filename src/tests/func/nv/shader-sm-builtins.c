// Copyright 2024 Collabora, Ltd.
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

#include "src/tests/func/nv/shader-sm-builtins-spirv.h"

struct sm_builtin_values {
    uint32_t warps_per_sm;
    uint32_t sm_count;
    uint32_t warp_id;
    uint32_t sm_id;
};

static void
test_sm_builtins(void)
{
    t_require_ext("VK_NV_shader_sm_builtins");

    VkPhysicalDeviceShaderSMBuiltinsPropertiesNV sm_builtins_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV,
    };
    VkPhysicalDeviceSubgroupProperties subgroup_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
        .pNext = &sm_builtins_props,
    };
    VkPhysicalDeviceProperties2 p = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &subgroup_props,
    };
    vkGetPhysicalDeviceProperties2(t_physical_dev, &p);

    // We assume subgroupSize == 32
    t_assert(subgroup_props.subgroupSize == 32);

    logi("shaderSMCount = %u", sm_builtins_props.shaderSMCount);
    logi("shaderWarpsPerSM = %u", sm_builtins_props.shaderWarpsPerSM);

    const uint32_t warps = sm_builtins_props.shaderSMCount *
                           sm_builtins_props.shaderWarpsPerSM;

    // We hope that using every available SM 16 times is enough to flood
    // the GPU.
    const uint32_t invocations = warps * 16;

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

    VkShaderModule cs = qoCreateShaderModuleGLSL(
        t_device, COMPUTE,
        QO_TARGET_ENV vulkan1.1
        QO_EXTENSION GL_KHR_shader_subgroup_basic : require
        QO_EXTENSION GL_NV_shader_sm_builtins : require

        layout(set = 0, binding = 0, std430) buffer Storage {
           uvec4 ua[];
        } ssbo;

        // Each workgroup is exactly one warp
        layout (local_size_x = 32) in;

        void main()
        {
            if (subgroupElect()) {
                ssbo.ua[gl_WorkGroupID.x] = uvec4(
                    gl_WarpsPerSMNV,
                    gl_SMCountNV,
                    gl_WarpIDNV,
                    gl_SMIDNV
                );
            }
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
                .flags = VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT,
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

    uint32_t ssbo_size = invocations * 16;
    VkBuffer buffer_out = qoCreateBuffer(t_device,
        .size = ssbo_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDeviceMemory mem_out = qoAllocBufferMemory(t_device, buffer_out,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer_out, mem_out, 0);
    const void *map = qoMapMemory(t_device, mem_out, 0, ssbo_size, 0);

    vkUpdateDescriptorSets(t_device,
        /*writeCount*/ 1,
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = buffer_out,
                    .offset = 0,
                    .range = ssbo_size,
                },
            },
        }, 0, NULL);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout, 0, 1,
                            &set, 0, NULL);

    vkCmdDispatch(t_cmd_buffer, invocations, 1, 1);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    bool *seen_sms = alloca(sm_builtins_props.shaderSMCount * sizeof(bool));
    memset(seen_sms, 0, sm_builtins_props.shaderSMCount * sizeof(bool));

    bool *seen_warps = alloca(sm_builtins_props.shaderWarpsPerSM * sizeof(bool));
    memset(seen_warps, 0, sm_builtins_props.shaderWarpsPerSM * sizeof(bool));

    const struct sm_builtin_values *values = map;
    for (uint32_t i = 0; i < invocations; i++) {
        t_assert(values[i].warps_per_sm == sm_builtins_props.shaderWarpsPerSM);
        t_assert(values[i].sm_count == sm_builtins_props.shaderSMCount);
        t_assert(values[i].warp_id < sm_builtins_props.shaderWarpsPerSM);
        seen_warps[values[i].warp_id] = true;
        t_assert(values[i].sm_id < sm_builtins_props.shaderSMCount);
        seen_sms[values[i].sm_id] = true;
    }

    bool missing_warp = false;
    for (uint32_t w = 0; w < sm_builtins_props.shaderWarpsPerSM; w++) {
        if (!seen_warps[w]) {
            missing_warp = true;
            logi("Never saw Warp %u", w);
        }
    }

    if (!missing_warp) {
        logi("Saw all advertised warps in the results");
    }

    bool missing_sm = false;
    for (uint32_t sm = 0; sm < sm_builtins_props.shaderSMCount; sm++) {
        if (!seen_sms[sm]) {
            missing_sm = true;
            logi("Never saw SM %u", sm);
        }
    }

    if (!missing_sm) {
        logi("Saw all advertised SMs in the results");
    }
}

test_define {
    .name = "func.nv.shader-sm-builtins",
    .start = test_sm_builtins,
    .no_image = true,
    .queue_setup = QUEUE_SETUP_COMPUTE,
};
