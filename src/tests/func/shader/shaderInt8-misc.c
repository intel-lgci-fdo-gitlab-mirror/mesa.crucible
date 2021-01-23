// Copyright 2020 Intel Corporation
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

#include "src/tests/func/shader/shaderInt8-misc-spirv.h"

#define SRC_LENGTH 64

static void
run_test(VkShaderModule fs, const void *data, unsigned data_size)
{
    VkShaderModule vs = qoCreateShaderModuleGLSL(t_device, VERTEX,
        layout(location = 0) in vec4 a_position;
        void main()
        {
            gl_Position = a_position;
        }
    );

    static const float vertices[8] = {
        -1.0, -1.0,
         1.0, -1.0,
        -1.0,  1.0,
         1.0,  1.0,
    };
    const unsigned vertices_offset = 0;

    const unsigned buffer_size = sizeof(vertices);

    VkBuffer buffer = qoCreateBuffer(t_device, .size = buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VkDeviceMemory mem = qoAllocBufferMemory(t_device, buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, buffer, mem, 0);

    char *map = qoMapMemory(t_device, mem, 0, buffer_size, 0);
    memcpy(map + vertices_offset, vertices, sizeof(vertices));

    /* Setup the buffer that will hold the data for the fragment shader. */
    VkBuffer fs_buffer = qoCreateBuffer(t_device, .size = data_size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkDeviceMemory fs_mem = qoAllocBufferMemory(t_device, fs_buffer,
        .properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    qoBindBufferMemory(t_device, fs_buffer, fs_mem, 0);

    map = (char *) qoMapMemory(t_device, fs_mem, 0, data_size, 0);
    memcpy(map, data, data_size);

    VkDescriptorSetLayout set_layout = qoCreateDescriptorSetLayout(t_device,
        .bindingCount = 1,
        .pBindings = (VkDescriptorSetLayoutBinding[]) {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
            },
        });

    VkDescriptorSet set = qoAllocateDescriptorSet(t_device,
        .descriptorPool = t_descriptor_pool,
        .pSetLayouts = &set_layout);

    vkUpdateDescriptorSets(t_device,
        1, /* writeCount */
        &(VkWriteDescriptorSet) {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo) {
                .buffer = fs_buffer,
                .offset = 0,
                .range = data_size,
            },
        }, 0, NULL);

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout);

    VkPipelineVertexInputStateCreateInfo vi_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
            .binding = 0,
            .stride = 2 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &(VkVertexInputAttributeDescription) {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = vertices_offset,
        },
    };

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .vertexShader = vs,
            .fragmentShader = fs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pVertexInputState = &vi_create_info,
            .flags = 0,
            .layout = pipeline_layout,
            .renderPass = t_render_pass,
            .subpass = 0,
        }});

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .renderPass = t_render_pass,
            .framebuffer = t_framebuffer,
            .renderArea = { { 0, 0 }, { t_width, t_height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = { 1.0, 0.0, 0.0, 1.0 } } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindVertexBuffers(t_cmd_buffer, 0, 1,
                           (VkBuffer[]) { buffer },
                           (VkDeviceSize[]) { vertices_offset });
    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(t_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, 0, 1, &set, 0, NULL);
    vkCmdDraw(t_cmd_buffer, /*vertexCount*/ 4, /*instanceCount*/ 1,
              /*firstVertex*/ 0, /*firstInstance*/ 0);
    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
}

static void
test_sign_int8_t(void)
{
    if (t_physical_dev_props->apiVersion < VK_API_VERSION_1_1)
        t_skipf("Vulkan 1.1 required");

    t_require_ext("VK_KHR_shader_float16_int8");

    VkPhysicalDeviceShaderFloat16Int8FeaturesKHR fp16_int8_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR
    };
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
	.pNext = &fp16_int8_features
    };

    vkGetPhysicalDeviceFeatures2(t_physical_dev, &features);

    if (!fp16_int8_features.shaderInt8)
        t_skipf("shaderInt8 not supported");

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int8: require

	layout(set = 0, binding = 0) uniform Data {
            i8vec2 data[256];
        };

        layout(location = 0) out vec4 f_color;

        void main()
        {
	    uint x = uint(gl_FragCoord.x);
	    uint y = uint(gl_FragCoord.y);
            uint i = (x & 15 | (y << 4)) % uint(data.length());

	    int8_t src = data[i].x;
	    int8_t expected = data[i].y;

	    int8_t result = sign(src);

	    f_color = (result == expected) ? vec4(0.0, 1.0, 0.0, 1.0)
		                           : vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    int8_t data[256 * 16];

    for (unsigned i = 0; i < 256; i++) {
	int8_t v = i;

	data[16 * i + 0] = v;

	if (v == 0) {
	    data[16 * i + 1] = 0;
	} else if (v < 0) {
	    data[16 * i + 1] = -1;
	} else {
	    data[16 * i + 1] = 1;
	}	    
    }

    run_test(fs, data, sizeof(data));
}

test_define {
    .name = "func.shader.sign.int8_t",
    .start = test_sign_int8_t,
    .image_filename = "32x32-green.ref.png",
};
