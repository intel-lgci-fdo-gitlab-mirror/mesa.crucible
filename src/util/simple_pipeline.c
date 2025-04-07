// Copyright 2015 Intel Corporation
// Copyright 2017 Valve Corporation
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

#include "util/simple_pipeline.h"
#include "util/misc.h"

#include "tapi/t.h"

#include "src/util/simple_pipeline-spirv.h"

#define GET_DEVICE_FUNCTION_PTR(name) \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(t_device, #name); \
    t_assert(name != NULL);

static void
ensure_subgroup_size_require(uint32_t required_subgroup_size,
                             VkShaderStageFlagBits stages)
{
    if (required_subgroup_size == 0)
        return;

    t_require_ext("VK_EXT_subgroup_size_control");

    VkPhysicalDeviceSubgroupSizeControlFeaturesEXT ssc_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT,
    };
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &ssc_features,
    };
    vkGetPhysicalDeviceFeatures2(t_physical_dev, &features);

    if (!ssc_features.subgroupSizeControl)
        t_skipf("subgroupSizeControl not supported, needed to require a subgroup size");

    VkPhysicalDeviceSubgroupSizeControlPropertiesEXT ssc_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &ssc_props,
    };
    vkGetPhysicalDeviceProperties2(t_physical_dev, &props);

    if ((ssc_props.requiredSubgroupSizeStages & stages) != stages)
        t_skipf("subgroupSizeControl not supported for Mesh shader stage");
    if ((ssc_props.requiredSubgroupSizeStages & stages) != stages)
        t_skipf("subgroupSizeControl not supported for Task shader stage");

    if (required_subgroup_size < ssc_props.minSubgroupSize) {
        t_skipf("requiredSubgroupSize (%u) smaller than minSubgroupSize (%u) supported",
                required_subgroup_size, ssc_props.minSubgroupSize);
    }
    if (required_subgroup_size > ssc_props.maxSubgroupSize) {
        t_skipf("requiredSubgroupSize (%u) larger than maxSubgroupSize (%u) supported",
                required_subgroup_size, ssc_props.maxSubgroupSize);
    }
}

void
run_simple_pipeline(VkShaderModule fs, void *push_constants,
                    size_t push_constants_size)
{
    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                QO_ATTACHMENT_DESCRIPTION_DEFAULTS,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
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
                .preserveAttachmentCount = 0,
            }
        });

    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        layout(location = 0) in vec4 a_position;
        void main()
        {
            gl_Position = a_position;
        }
    );

    VkPipelineVertexInputStateCreateInfo vi_info = {
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
        }
    };

    VkPipelineLayout layout;
    if (push_constants_size) {
        VkPushConstantRange constants = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = push_constants_size
        };

        layout = qoCreatePipelineLayout(t_device,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &constants);
    } else {
        layout = qoCreatePipelineLayout(t_device, .pushConstantRangeCount = 0);
    }

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .vertexShader = vs,
            .fragmentShader = fs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
                QO_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO_DEFAULTS,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            },
            .pVertexInputState = &vi_info,
            .renderPass = pass,
            .layout = layout,
            .subpass = 0,
        }});

    const float vertices[] = {
        -1.0, 1.0,
        1.0, 1.0,
        -1.0, -1.0,
        1.0, -1.0
    };

    VkBuffer vb = qoCreateBuffer(t_device, .size =  sizeof(vertices),
                                 .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkDeviceMemory vb_mem = qoAllocBufferMemory(t_device, vb,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    qoBindBufferMemory(t_device, vb, vb_mem, 0);

    void *vb_map = qoMapMemory(t_device, vb_mem, 0, sizeof(vertices), 0);

    memcpy(vb_map, vertices, sizeof(vertices));

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = t_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = {1.0, 0.0, 0.0, 1.0} } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    if (push_constants_size) {
        vkCmdPushConstants(t_cmd_buffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, push_constants_size, push_constants);
    }

    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 1, (VkBuffer[]) { vb },
                           (VkDeviceSize[]) { 0 });

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdDraw(t_cmd_buffer, 4, 1, 0, 0);

    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}


void run_simple_compute_pipeline(VkShaderModule cs,
                                 const simple_compute_pipeline_options_t *opts)
{
    if (opts->required_subgroup_size) {
        ensure_subgroup_size_require(opts->required_subgroup_size,
                                     VK_SHADER_STAGE_COMPUTE_BIT);
    }

    bool has_push_constants = opts->push_constants_size > 0;
    bool has_storage = opts->storage_size > 0;

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

    VkPushConstantRange constant_range = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = opts->push_constants_size,
    };

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = has_storage ? 1 : 0,
        .pSetLayouts = has_storage ? &set_layout : NULL,
        .pushConstantRangeCount = has_push_constants ? 1 : 0,
        .pPushConstantRanges = has_push_constants ? &constant_range : NULL);


    VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT sgs_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT,
        .requiredSubgroupSize = opts->required_subgroup_size,
    };

    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
        &(VkComputePipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = opts->required_subgroup_size ? &sgs_info : NULL,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = cs,
                .pName = "main",
            },
            .flags = 0,
            .layout = pipeline_layout
        }, NULL, &pipeline);
    t_assert(result == VK_SUCCESS);
    t_cleanup_push_vk_pipeline(t_device, pipeline);

    VkDescriptorSet set = qoAllocateDescriptorSet(t_device,
        .descriptorPool = t_descriptor_pool,
        .pSetLayouts = &set_layout);

    VkBuffer storage_buf;
    VkDeviceMemory storage_mem;
    if (has_storage) {
        storage_buf = qoCreateBuffer(t_device,
            .size = opts->storage_size,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        storage_mem = qoAllocBufferMemory(t_device, storage_buf,
            .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        qoBindBufferMemory(t_device, storage_buf, storage_mem, 0);

        // Copy storage to buffer.
        void *storage_ptr = NULL;
        VkResult result = vkMapMemory(t_device, storage_mem, 0,
                                      opts->storage_size, 0, &storage_ptr);
        t_assert(result == VK_SUCCESS);
        memcpy(storage_ptr, opts->storage, opts->storage_size);
        vkUnmapMemory(t_device, storage_mem);

        vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                             0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = storage_buf,
                .offset = 0,
                .size = opts->storage_size,
            }, 0, NULL);

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
                        .buffer = storage_buf,
                        .offset = 0,
                        .range = opts->storage_size,
                    },
                },
            }, 0, NULL);
    }

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    if (has_storage) {
        vkCmdBindDescriptorSets(t_cmd_buffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline_layout, 0, 1,
            &set, 0, NULL);
    }

    if (has_push_constants) {
        vkCmdPushConstants(t_cmd_buffer, pipeline_layout,
            VK_SHADER_STAGE_COMPUTE_BIT, 0,
            opts->push_constants_size, opts->push_constants);
    }

    vkCmdDispatch(t_cmd_buffer, MAX(1, opts->x_count),
                  MAX(1, opts->y_count), MAX(1, opts->z_count));

    if (has_storage) {
        vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .buffer = storage_buf,
                .offset = 0,
                .size = opts->storage_size,
            }, 0, NULL);
    }

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    // Copy storage back.
    if (has_storage) {
        void *storage_ptr = NULL;
        VkResult result = vkMapMemory(t_device, storage_mem, 0,
                                      opts->storage_size, 0, &storage_ptr);
        t_assert(result == VK_SUCCESS);
        memcpy(opts->storage, storage_ptr, opts->storage_size);
        vkUnmapMemory(t_device, storage_mem);

        vkFreeMemory(t_device, storage_mem, NULL);
    }
}

void
run_simple_mesh_pipeline(VkShaderModule mesh,
                         const struct simple_mesh_pipeline_options *_opts)
{
    struct simple_mesh_pipeline_options opts = {};
    if (_opts)
        opts = *_opts;

    if (opts.type == UNKNOWN_MESH_SHADER) {
        if (strncmp(t_name, "func.mesh.nv.", strlen("func.mesh.nv.")) == 0)
            opts.type = NV_MESH_SHADER;
        else if (strncmp(t_name, "func.mesh.ext.", strlen("func.mesh.ext.")) == 0)
            opts.type = EXT_MESH_SHADER;
        else
            t_assert(!"unknown mesh shader extension");
    }

    t_assert(opts.type == NV_MESH_SHADER || opts.type == EXT_MESH_SHADER);

    enum VkShaderStageFlagBits vk_shader_stage_mesh_bit;
    enum VkShaderStageFlagBits vk_shader_stage_task_bit;

    VkPhysicalDeviceMeshShaderFeaturesNV mesh_features_nv = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV,
    };
    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_features_ext = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
    };

    VkPhysicalDeviceSubgroupSizeControlFeaturesEXT ssc_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT,
    };

    if (opts.group_count_x == 0)
        opts.group_count_x = 1;
    if (opts.group_count_y == 0)
        opts.group_count_y = 1;
    if (opts.group_count_z == 0)
        opts.group_count_z = 1;

    if (opts.type == NV_MESH_SHADER) {
        t_require_ext("VK_NV_mesh_shader");
        t_assert(opts.group_count_y == 1);
        t_assert(opts.group_count_z == 1);
        vk_shader_stage_mesh_bit = VK_SHADER_STAGE_MESH_BIT_NV;
        vk_shader_stage_task_bit = VK_SHADER_STAGE_TASK_BIT_NV;
        ssc_features.pNext = &mesh_features_nv;
    } else if (opts.type == EXT_MESH_SHADER) {
        t_require_ext("VK_EXT_mesh_shader");
        vk_shader_stage_mesh_bit = VK_SHADER_STAGE_MESH_BIT_EXT;
        vk_shader_stage_task_bit = VK_SHADER_STAGE_TASK_BIT_EXT;
        ssc_features.pNext = &mesh_features_ext;
    }

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &ssc_features,
    };
    vkGetPhysicalDeviceFeatures2(t_physical_dev, &features);

    VkBool32 task_shader_supported = false;
    VkBool32 mesh_shader_supported = false;

    if (opts.type == NV_MESH_SHADER) {
        mesh_shader_supported = mesh_features_nv.meshShader;
        task_shader_supported = mesh_features_nv.taskShader;
    } else if (opts.type == EXT_MESH_SHADER) {
        mesh_shader_supported = mesh_features_ext.meshShader;
        task_shader_supported = mesh_features_ext.taskShader;
    }

    if (!mesh_shader_supported)
        t_skipf("meshShader not supported");
    if (!task_shader_supported && opts.task != VK_NULL_HANDLE)
        t_skipf("taskShader not supported");

    if (opts.required_subgroup_size) {
        ensure_subgroup_size_require(
            opts.required_subgroup_size,
            vk_shader_stage_mesh_bit |
            (opts.task != VK_NULL_HANDLE ? vk_shader_stage_task_bit : 0));
    }

    const bool no_image = t_no_image;

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = no_image ? 0 : 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                QO_ATTACHMENT_DESCRIPTION_DEFAULTS,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            },
        },
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                QO_SUBPASS_DESCRIPTION_DEFAULTS,
                .colorAttachmentCount = no_image ? 0 : 1,
                .pColorAttachments = (VkAttachmentReference[]) {
                    {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    },
                },
                .preserveAttachmentCount = 0,
            }
        });

    bool has_storage = opts.storage_size > 0;
    bool has_uniform = opts.uniform_data_size > 0;
    bool has_push_constants = opts.push_constants_size > 0;

    VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding[2];
    VkDescriptorSetLayout set_layouts[2];
    VkDescriptorSet sets[2];
    VkDeviceMemory storage_mem;

    VkWriteDescriptorSet vkWriteDescriptorSet[2];
    VkDescriptorBufferInfo vkDescriptorBufferInfo[2];

    uint32_t descSetCount = 0;
    VkBuffer storage_buf;
    if (has_storage) {
        vkDescriptorSetLayoutBinding[descSetCount] =
        (VkDescriptorSetLayoutBinding) {
                .binding = descSetCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = vk_shader_stage_task_bit |
                              vk_shader_stage_mesh_bit |
                              VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
        };

        set_layouts[descSetCount] = qoCreateDescriptorSetLayout(t_device,
            .bindingCount = 1,
            .pBindings = &vkDescriptorSetLayoutBinding[descSetCount]
        );

        sets[descSetCount] = qoAllocateDescriptorSet(t_device,
            .descriptorPool = t_descriptor_pool,
            .pSetLayouts = &set_layouts[descSetCount]);

        storage_buf = qoCreateBuffer(t_device,
            .size = opts.storage_size,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        storage_mem = qoAllocBufferMemory(t_device, storage_buf,
            .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        qoBindBufferMemory(t_device, storage_buf, storage_mem, 0);

        // Copy storage to buffer.
        void *storage_ptr = NULL;
        VkResult result = vkMapMemory(t_device, storage_mem, 0,
                                      opts.storage_size, 0, &storage_ptr);
        t_assert(result == VK_SUCCESS);
        memcpy(storage_ptr, opts.storage, opts.storage_size);
        vkUnmapMemory(t_device, storage_mem);

        vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
                             0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = storage_buf,
                .offset = 0,
                .size = opts.storage_size,
            }, 0, NULL);

        vkDescriptorBufferInfo[descSetCount] = (VkDescriptorBufferInfo) {
            .buffer = storage_buf,
            .offset = 0,
            .range = opts.storage_size,
        };

        vkWriteDescriptorSet[descSetCount] = (VkWriteDescriptorSet) {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = sets[descSetCount],
                .dstBinding = descSetCount,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &vkDescriptorBufferInfo[descSetCount],
            };

        descSetCount++;
    }

    VkDeviceMemory uniform_mem;
    if (has_uniform) {
        vkDescriptorSetLayoutBinding[descSetCount] =
        (VkDescriptorSetLayoutBinding) {
                .binding = descSetCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = vk_shader_stage_task_bit |
                              vk_shader_stage_mesh_bit |
                              VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
        };

        set_layouts[descSetCount] = qoCreateDescriptorSetLayout(t_device,
            .bindingCount = 1,
            .pBindings = &vkDescriptorSetLayoutBinding[descSetCount]
        );

        sets[descSetCount] = qoAllocateDescriptorSet(t_device,
            .descriptorPool = t_descriptor_pool,
            .pSetLayouts = &set_layouts[descSetCount]);

        VkBuffer uniform_buf = qoCreateBuffer(t_device,
            .size = opts.uniform_data_size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        uniform_mem = qoAllocBufferMemory(t_device, uniform_buf,
            .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        qoBindBufferMemory(t_device, uniform_buf, uniform_mem, 0);

        // Copy uniform to buffer.
        void *uniform_ptr = NULL;
        VkResult result = vkMapMemory(t_device, uniform_mem, 0,
                                      opts.uniform_data_size, 0, &uniform_ptr);
        t_assert(result == VK_SUCCESS);
        memcpy(uniform_ptr, opts.uniform_data, opts.uniform_data_size);
        vkUnmapMemory(t_device, uniform_mem);

        vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
                             0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = uniform_buf,
                .offset = 0,
                .size = opts.uniform_data_size,
            }, 0, NULL);

        vkDescriptorBufferInfo[descSetCount] = (VkDescriptorBufferInfo) {
            .buffer = uniform_buf,
            .offset = 0,
            .range = opts.uniform_data_size,
        };

        vkWriteDescriptorSet[descSetCount] = (VkWriteDescriptorSet) {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = sets[descSetCount],
                .dstBinding = descSetCount,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &vkDescriptorBufferInfo[descSetCount],
            };

        descSetCount++;
    }

    VkPushConstantRange push_range;
    if (has_push_constants) {
        push_range.stageFlags = vk_shader_stage_mesh_bit;
        push_range.offset = 0;
        push_range.size = opts.push_constants_size;
    }

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = descSetCount,
        .pSetLayouts = descSetCount ? set_layouts : NULL,
        .pushConstantRangeCount = has_push_constants ? 1 : 0,
        .pPushConstantRanges = has_push_constants ? &push_range : NULL
    );

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .taskShader = opts.task,
            .meshShader = mesh,
            .fragmentShader = opts.fs,
            .meshRequiredSubgroupSize = opts.required_subgroup_size,
            .taskRequiredSubgroupSize = opts.required_subgroup_size,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .renderPass = pass,
            .layout = pipeline_layout,
            .subpass = 0,
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
                QO_PIPELINE_RASTERIZATION_STATE_CREATE_INFO_DEFAULTS,
                .rasterizerDiscardEnable = no_image,
            },
            .pViewportState = opts.viewport_state,
            .stageCount = opts.mesh_create_info ? 1 : 0,
            .pStages = opts.mesh_create_info ? opts.mesh_create_info : NULL,
        }});

    if (descSetCount)
        vkUpdateDescriptorSets(t_device, descSetCount, vkWriteDescriptorSet, 0, NULL);

    uint32_t width, height;
    VkFramebuffer framebuffer;
    if (no_image) {
        // Pick a size, make this an option if we care later.
        width = 128;
        height = 128;
        framebuffer = qoCreateFramebuffer(t_device,
            .renderPass = pass,
            .width = width,
            .height = height,
            .layers = 1,
            .attachmentCount = 0,
            .pAttachments = 0);
    } else {
        width = t_width;
        height = t_height;
        framebuffer = t_framebuffer;
    }

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = framebuffer,
            .renderArea = { { 0, 0 }, { width, height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = {0.3, 0.3, 0.3, 1.0} } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    if (descSetCount) {
        vkCmdBindDescriptorSets(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, descSetCount, sets, 0, NULL);
    }

    if (has_push_constants) {
        vkCmdPushConstants(t_cmd_buffer, pipeline_layout,
                vk_shader_stage_mesh_bit, 0, opts.push_constants_size,
                opts.push_constants);
    }

    if (opts.type == NV_MESH_SHADER) {
        GET_DEVICE_FUNCTION_PTR(vkCmdDrawMeshTasksNV);
        vkCmdDrawMeshTasksNV(t_cmd_buffer, opts.group_count_x, 0);
    } else if (opts.type == EXT_MESH_SHADER) {
        GET_DEVICE_FUNCTION_PTR(vkCmdDrawMeshTasksEXT);
        vkCmdDrawMeshTasksEXT(t_cmd_buffer, opts.group_count_x,
                opts.group_count_y, opts.group_count_z);
    }

    vkCmdEndRenderPass(t_cmd_buffer);

    if (has_storage) {
        vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1,
            &(VkBufferMemoryBarrier) {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                .buffer = storage_buf,
                .offset = 0,
                .size = opts.storage_size,
            }, 0, NULL);
    }

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    // Copy storage back.
    if (has_storage) {
        void *storage_ptr = NULL;
        VkResult result = vkMapMemory(t_device, storage_mem, 0,
                                      opts.storage_size, 0, &storage_ptr);
        t_assert(result == VK_SUCCESS);
        memcpy(opts.storage, storage_ptr, opts.storage_size);
        vkUnmapMemory(t_device, storage_mem);

        vkFreeMemory(t_device, storage_mem, NULL);
    }

    if (has_uniform) {
        vkFreeMemory(t_device, uniform_mem, NULL);
    }
}
