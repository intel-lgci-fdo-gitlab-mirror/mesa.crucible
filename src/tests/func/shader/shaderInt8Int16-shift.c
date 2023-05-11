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

#include "src/tests/func/shader/shaderInt8Int16-shift-spirv.h"

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
test_shift_int16_t(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE src_length 64

	layout(set = 0, binding = 0) uniform Data {
            ivec4 data[src_length];
        };

        layout(location = 0) out vec4 f_color;

        void main()
        {
	    uint x = uint(gl_FragCoord.x);
	    uint y = uint(gl_FragCoord.y);
            uint i = (x | (y << 5)) % uint(src_length);
            uint j = (y >> 1) % 6u;

	    i16vec2 a = unpack16(data[i].x);
	    i16vec4 b = i16vec4(unpack16(data[i].y),
				unpack16(data[i].z));
	    i16vec2 c = unpack16(data[i].w);
	    int16_t result;
	    int16_t expected;

	    switch (j) {
	    case 0:
		result = a.x << (a.y & 7);
		expected = b.x;
		break;
	    case 1:
		result = a.x >> (a.y & 7);
		expected = b.y;
		break;
	    case 2:
		result = int16_t(uint16_t(a.x) >> (a.y & 7));
		expected = b.z;
		break;
	    case 3:
		result = a.x << (a.y & 15);
		expected = b.w;
		break;
	    case 4:
		result = a.x >> (a.y & 15);
		expected = c.x;
		break;
	    case 5:
		result = int16_t(uint16_t(a.x) >> (a.y & 15));
		expected = c.y;
		break;
	    }

	    f_color = (result == expected) ? vec4(0.0, 1.0, 0.0, 1.0)
		                           : vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    static const unsigned src[SRC_LENGTH] = {
	    3,   613,  1399,  2237,  3089,  4003,  4951,  5867,
	 6857,  7853,  8839,  9829, 10867, 11941, 12953, 13999,
        15083, 16087, 17207, 18253, 19429, 20479, 21577, 22669,
	23753, 24907, 26017, 27103, 28297, 29387, 30593, 31727,
	33923, 34511, 35099, 36299, 37363, 37423, 38653, 39791,
	41011, 42089, 43019, 43207, 44483, 45667, 46861, 48079,
	49009, 49253, 50387, 51607, 52837, 54011, 55229, 56443,
	57559, 58733, 59929, 61223, 62473, 63659, 64969, 65521,
    };

    int16_t data[SRC_LENGTH * 8];

    for (unsigned i = 0; i < SRC_LENGTH; i++) {
	unsigned signed_src = src[i];
	if (signed_src & 0x8000)
	    signed_src |= 0xffff0000;

        /* On Intel GPUs, the int16_t shift count is implicitly masked with
         * 0x1f (instead of 0x0f). Munge the shift count with a value that has
         * 0x10 set.
         */
        unsigned shift_count = (i & 15) | 0x5550;

	data[(i * 8) + 0] = src[i];
	data[(i * 8) + 1] = shift_count;

	data[(i * 8) + 2] = src[i] << (shift_count & 7);
	data[(i * 8) + 3] = signed_src >> (shift_count & 7);
	data[(i * 8) + 4] = src[i] >> (shift_count & 7);

	data[(i * 8) + 5] = src[i] << (shift_count & 15);
	data[(i * 8) + 6] = signed_src >> (shift_count & 15);
	data[(i * 8) + 7] = src[i] >> (shift_count & 15);
    }

    run_test(fs, data, sizeof(data));
}

test_define {
    .name = "func.shader.shift.int16_t",
    .start = test_shift_int16_t,
    .image_filename = "32x32-green.ref.png",
};

static void
test_shift_int8_t(void)
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

        QO_DEFINE src_length 64

	layout(set = 0, binding = 0) uniform Data {
            ivec2 data[src_length];
        };

        layout(location = 0) out vec4 f_color;

        void main()
        {
	    uint x = uint(gl_FragCoord.x);
	    uint y = uint(gl_FragCoord.y);
            uint i = (x | (y << 5)) % uint(src_length);
            uint j = (y >> 1) % 6u;

	    i8vec4 a = unpack8(data[i].x);
	    i8vec4 b = unpack8(data[i].y);
	    int8_t result;
	    int8_t expected;

	    switch (j) {
	    case 0:
		result = a.x << (a.y & 3);
		expected = a.z;
		break;
	    case 1:
		result = a.x >> (a.y & 3);
		expected = a.w;
		break;
	    case 2:
		result = int8_t(uint8_t(a.x) >> (a.y & 3));
		expected = b.x;
		break;
	    case 3:
		result = a.x << (a.y & 7);
		expected = b.y;
		break;
	    case 4:
		result = a.x >> (a.y & 7);
		expected = b.z;
		break;
	    case 5:
		result = int8_t(uint8_t(a.x) >> (a.y & 7));
		expected = b.w;
		break;
	    }

	    f_color = (result == expected) ? vec4(0.0, 1.0, 0.0, 1.0)
		                           : vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    static const unsigned src[SRC_LENGTH] = {
	2,   3,   5,   7,  11,  13,  17,  19,
       23,  29,  31,  37,  41,  43,  47,  53,
       59,  61,  67,  71,  73,  79,  83,  89,
       97, 101, 103, 107, 109, 113, 127, 131,

      137, 139, 149, 151, 157, 163, 167, 173,
      179, 181, 191, 193, 197, 199, 211, 223,
      227, 229, 233, 239, 241, 251,

      /* There aren't enough primes <= 255, so supply a few more values to
       * fill out the data set. :)
       */
        0, 255,
        1,   4,   8,  16,  32,  64, 128, 0x55
    };

    int8_t data[SRC_LENGTH * 8];

    for (unsigned i = 0; i < SRC_LENGTH; i++) {
	unsigned signed_src = src[i];
	if (signed_src & 0x80)
	    signed_src |= 0xffffff00;

        /* On Intel GPUs, the int8_t shift count is implicitly masked with
         * 0x1f (instead of 0x07). Munge the shift count with a value that has
         * 0x18 set.
         */
        unsigned shift_count = (i & 7) | 0x58;

	data[(i * 8) + 0] = src[i];
	data[(i * 8) + 1] = shift_count;

	data[(i * 8) + 2] = src[i] << (shift_count & 3);
	data[(i * 8) + 3] = signed_src >> (shift_count & 3);
	data[(i * 8) + 4] = src[i] >> (shift_count & 3);

	data[(i * 8) + 5] = src[i] << (shift_count & 7);
	data[(i * 8) + 6] = signed_src >> (shift_count & 7);
	data[(i * 8) + 7] = src[i] >> (shift_count & 7);
    }

    run_test(fs, data, sizeof(data));
}

test_define {
    .name = "func.shader.shift.int8_t",
    .start = test_shift_int8_t,
    .image_filename = "32x32-green.ref.png",
};
