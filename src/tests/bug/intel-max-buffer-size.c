// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"

/*
 * This is a test verifies the exposed limit for maxStorageBufferRange
 * is accurate.
 *
 * The test create a sparse buffer of the maximum size and maps a
 * 64KiB page at the end of the buffer then a compute shader writes
 * the last visible dword accoding the the reported limit and another
 * compute shader reads back the value.
 */

#include "src/tests/bug/intel-max-buffer-size-spirv.h"

#define TEST_VALUE        0x42424242u

typedef struct {
    uint64_t buffer_size;
    uint64_t page_offset;
    uint64_t page_size;

    uint32_t dword_index;

    VkBuffer sparse_buf;
    VkDeviceMemory sparse_mem;

    VkBuffer host_buf;
    VkDeviceMemory host_mem;

    VkPipeline write_pipeline;
    VkPipeline read_pipeline;
    VkPipelineLayout pipeline_layout;

    VkDescriptorSet write_set;
    VkDescriptorSet read_set;

    VkDescriptorSetLayout set_layout;

    VkDeviceSize sparse_alignment;
} CTX;

static void
create_sparse_buffer(CTX *ctx)
{
    VkMemoryRequirements req;

    ctx->sparse_buf = qoCreateBuffer(t_device,
        .size = ctx->buffer_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
                 VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT);

    vkGetBufferMemoryRequirements(t_device, ctx->sparse_buf, &req);

    ctx->sparse_alignment = req.alignment;

    t_assertf(ctx->sparse_alignment == ctx->page_size,
              "unexpected sparse buffer granularity: %llu, "
              "expected %llu",
              (unsigned long long)ctx->sparse_alignment,
              (unsigned long long)ctx->page_size);

    ctx->sparse_mem = qoAllocMemory(t_device,
        .allocationSize = ctx->page_size,
        .memoryTypeIndex = qoFindMemoryTypeWithProperties(~0u, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
}

static void
bind_sparse_buffer(CTX *ctx)
{
    VkSparseMemoryBind sparse_bind = {
        .resourceOffset = ctx->page_offset,
        .size = ctx->page_size,
        .memory = ctx->sparse_mem,
        .memoryOffset = 0,
        .flags = 0,
    };

    VkSparseBufferMemoryBindInfo buffer_bind = {
        .buffer = ctx->sparse_buf,
        .bindCount = 1,
        .pBinds = &sparse_bind,
    };

    VkBindSparseInfo bind_info = {
        .sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .bufferBindCount = 1,
        .pBufferBinds = &buffer_bind,
        .imageOpaqueBindCount = 0,
        .pImageOpaqueBinds = NULL,
        .imageBindCount = 0,
        .pImageBinds = NULL,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };

    VkResult result = vkQueueBindSparse(t_queue, 1, &bind_info,
                                        VK_NULL_HANDLE);
    t_assert(result == VK_SUCCESS);

    /*
     * vkQueueBindSparse is not automatically ordered against command
     * buffer execution, so wait for the sparse binding operation before
     * submitting the compute work.
     */
    vkQueueWaitIdle(t_queue);
}


/*
 * Small host-visible SSBO used to get the value back to the CPU.
 */
static void
create_host_buffer(CTX *ctx)
{
    ctx->host_buf = qoCreateBuffer(t_device,
        .size = sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    ctx->host_mem = qoAllocBufferMemory(t_device, ctx->host_buf,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    qoBindBufferMemory(t_device, ctx->host_buf, ctx->host_mem, 0);
}


static void
create_pipelines(CTX *ctx)
{
    ctx->set_layout = qoCreateDescriptorSetLayout(t_device,
        .bindingCount = 2,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
        });

    ctx->pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &ctx->set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = 4,
        });

    /*
     * Shader 1:
     *
     * The descriptor for binding 0 points directly at the final
     * uint32_t of the sparse buffer.
     */
    VkShaderModule write_shader = qoCreateShaderModuleGLSL(
        t_device, COMPUTE,

        layout(local_size_x = 1) in;

        layout(push_constant, std430) uniform Push {
          uint index;
        };

        layout(set = 0, binding = 0, std430) buffer Sparse {
            uint values[];
        } sparse;

        void main()
        {
            sparse.values[index] = 0x42424242u;
        }
    );

    VkShaderModule read_shader = qoCreateShaderModuleGLSL(
        t_device, COMPUTE,

        layout(local_size_x = 1) in;

        layout(push_constant, std430) uniform Push {
          uint index;
        };

        layout(set = 0, binding = 0, std430) readonly buffer Sparse {
            uint values[];
        } sparse;

        layout(set = 0, binding = 1, std430) buffer Host {
            uint value;
        } host;

        void main()
        {
            host.value = sparse.values[index];
        }
    );

    VkResult result;

    result = vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
        &(VkComputePipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = {
                .sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = write_shader,
                .pName = "main",
            },
            .layout = ctx->pipeline_layout,
        }, NULL, &ctx->write_pipeline);
    t_assert(result == VK_SUCCESS);
    t_cleanup_push_vk_pipeline(t_device, ctx->write_pipeline);

    result = vkCreateComputePipelines(t_device, t_pipeline_cache, 1,
        &(VkComputePipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = {
                .sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = read_shader,
                .pName = "main",
            },
            .layout = ctx->pipeline_layout,
        }, NULL, &ctx->read_pipeline);
    t_assert(result == VK_SUCCESS);
    t_cleanup_push_vk_pipeline(t_device, ctx->read_pipeline);

    ctx->write_set =
        qoAllocateDescriptorSet(t_device,
                                .descriptorPool = t_descriptor_pool,
                                .pSetLayouts = &ctx->set_layout);

    ctx->read_set =
        qoAllocateDescriptorSet(t_device,
                                .descriptorPool = t_descriptor_pool,
                                .pSetLayouts = &ctx->set_layout);

    /*
     * The sparse buffer is 4 GiB large, but only the last page is
     * resident.  The descriptor offset is the last uint32_t.
     */
    vkUpdateDescriptorSets(t_device, 1,
        &(VkWriteDescriptorSet) {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = ctx->write_set,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                .buffer = ctx->sparse_buf,
                .offset = 0,
                .range = ctx->buffer_size,
            },
        }, 0, NULL);

    /*
     * Read shader:
     *
     *   binding 0 -> final 4 bytes of sparse buffer
     *   binding 1 -> host-visible result buffer
     */
    vkUpdateDescriptorSets(t_device, 2,
        (VkWriteDescriptorSet[]) {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = ctx->read_set,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = ctx->sparse_buf,
                    .offset = 0,
                    .range = ctx->buffer_size,
                },
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = ctx->read_set,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &(VkDescriptorBufferInfo) {
                    .buffer = ctx->host_buf,
                    .offset = 0,
                    .range = sizeof(uint32_t),
                },
            },
        }, 0, NULL);
}


static void
dispatch_write(CTX *ctx)
{
    vkCmdBindPipeline(t_cmd_buffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->write_pipeline);

    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->pipeline_layout,
                            0, 1,
                            &ctx->write_set,
                            0, NULL);

    vkCmdPushConstants(t_cmd_buffer, ctx->pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0, 4, &ctx->dword_index);

    vkCmdDispatch(t_cmd_buffer, 1, 1, 1);

    vkCmdPipelineBarrier(t_cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                         &(VkMemoryBarrier) {
                           .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                           .pNext = NULL,
                           .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                           .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
                         }, 0, NULL, 0, NULL);
}


static void
dispatch_read(CTX *ctx)
{
    vkCmdBindPipeline(t_cmd_buffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      ctx->read_pipeline);

    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            ctx->pipeline_layout,
                            0, 1,
                            &ctx->read_set,
                            0, NULL);

    vkCmdPushConstants(t_cmd_buffer, ctx->pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0, 4, &ctx->dword_index);

    vkCmdDispatch(t_cmd_buffer, 1, 1, 1);
}


static void
verify_result(CTX *ctx)
{
    uint32_t *value = NULL;

    VkResult result = vkMapMemory(t_device,
                                  ctx->host_mem,
                                  0,
                                  sizeof(uint32_t),
                                  0,
                                  (void **)&value);
    t_assert(result == VK_SUCCESS);

    t_assertf(*value == TEST_VALUE,
              "unexpected value: found 0x%08x, expected 0x%08x",
              *value, TEST_VALUE);

    vkUnmapMemory(t_device, ctx->host_mem);
}

#define ALIGN(val, align) (((val) + (align) - 1) & ~((align) - 1))

static void
test(void)
{
    CTX ctx = {
      .buffer_size = ALIGN((uint64_t)t_physical_dev_props->limits.maxStorageBufferRange, 4),
      .page_size = 64 * 1024,
      .dword_index = ALIGN((uint64_t)t_physical_dev_props->limits.maxStorageBufferRange, 4) / 4 - 1,
    };

    ctx.page_offset = ALIGN(ctx.buffer_size - ctx.page_size, ctx.page_size);

    create_sparse_buffer(&ctx);
    create_host_buffer(&ctx);

    bind_sparse_buffer(&ctx);

    create_pipelines(&ctx);

    /*
     * Write 0x42424242 into the final four bytes of the
     * physically-backed 64 KiB page.
     */
    dispatch_write(&ctx);

    /*
     * Read those same four bytes with another compute shader
     * and write them into host-visible memory.
     */
    dispatch_read(&ctx);

    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    verify_result(&ctx);

    t_pass();
}


test_define {
    .name = "bug.intel-max-buffer-size",
    .features = {
        .sparseBinding = true,
        .sparseResidencyBuffer = true,
    },
    .start = test,
    .no_image = true,
    .queue_setup = QUEUE_SETUP_COMPUTE,
};
