// Copyright 2019 Intel Corporation
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
#include "tapi/t.h"
#include "intel_shader_integer_functions2_common.h"

#include "src/tests/func/intel_shader_integer_functions2/absoluteDifference-spirv.h"

#define abs_sub_template(name, output_type, input_type)              \
static void                                                          \
name(void *_dest, unsigned dest_index,                               \
     const void *_src_data, unsigned i, unsigned j)                  \
{                                                                    \
    output_type *dest = (output_type *) _dest;                       \
    const input_type *src_data = (const input_type *) _src_data;     \
    const input_type a = src_data[i];                                \
    const input_type b = src_data[j];                                \
                                                                     \
    dest[dest_index] = (a > b) ? a - b : b - a;                      \
}

abs_sub_template(abs_isub16, uint16_t, int16_t);
abs_sub_template(abs_usub16, uint16_t, uint16_t);
abs_sub_template(abs_isub32, uint32_t, int32_t);
abs_sub_template(abs_usub32, uint32_t, uint32_t);
abs_sub_template(abs_isub64, uint64_t, int64_t);
abs_sub_template(abs_usub64, uint64_t, uint64_t);

/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 2 bytes.  This results in a maximum of
 * 8192 components.  absoluteDifference is commutative, so instead of
 * explicitly storing N^2 results, we store N+(N-1)+(N-2)+...+1 = (N+1)*N/2
 * results.  The total storage requirement is (N+1)*N/2+N.  N=126 would
 * require 8127 components, and N=127 would require 8255 components.
 *
 * The storage requirement can be further reduced by observing that the
 * diagonal of the result matrix is always 0 because absoluteDifference(x, x)
 * == 0.  The new total storage requirement is (((N+1)*N/2)-N)+N (because the
 * diagonal is N elements).  N=127 would require 8128 components, and N=128
 * would require 8256 components.  One more test vector!  TOTALLY WORTH IT!
 */
static const int16_t src_16bit[] = {
    0x8000, 0x8001, 0xc000, 0xc001, 0xe000, 0xe001, 0xf000, 0xf001,
    0xf800, 0xf801, 0xfc00, 0xfc01, 0xfe00, 0xfe01, 0xff00, 0xff01,
    0xff80, 0xff81, 0xffc0, 0xffc1, 0xffe0, 0xffe1, 0xfff0, 0xfff1,
    0xfff8, 0xfff9, 0xfffc, 0xfffd, 0xfffe, 0xffff, 0x0000,         // 31

    0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
    0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018,
    0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038,
    0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, // 63

    0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8,
    0x01f1, 0x01f2, 0x01f3, 0x01f4, 0x01f5, 0x01f6, 0x01f7, 0x01f8,
    0x03f1, 0x03f2, 0x03f3, 0x03f4, 0x03f5, 0x03f6, 0x03f7, 0x03f8,
    0x07f1, 0x07f2, 0x07f3, 0x07f4, 0x07f5, 0x07f6, 0x07f7, 0x07f8, // 95

    0x0ff1, 0x0ff2, 0x0ff3, 0x0ff4, 0x0ff5, 0x0ff6, 0x0ff7, 0x0ff8,
    0x1ff1, 0x1ff2, 0x1ff3, 0x1ff4, 0x1ff5, 0x1ff6, 0x1ff7, 0x1ff8,
    0x3ff1, 0x3ff2, 0x3ff3, 0x3ff4, 0x3ff5, 0x3ff6, 0x3ff7, 0x3ff8,
    0x7ff1, 0x7ff2, 0x7ff3, 0x7ff4, 0x7ff5, 0x7ff6, 0x7ff7, 0x7ff8, // 127
};

static void
absoluteDifference_int16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 127;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len + int(SUM_N_to_1(len)) - len + 7) / 8];
        };

        int16_t get_word(uint i)
        {
            return int16_t(bitfieldExtract(data[i / 8u][(i % 8u) / 2u],
                                           int((i % 2u) * 16u),
                                           16));
        }

        uint16_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return uint16_t(0);

            /* Ironically, max(i, j) - min(i, j) == absoluteDifference(i, j). */
            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return uint16_t(get_word(idx));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int16_t a = get_word(i);
            int16_t b = get_word(j);
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 127);

    const unsigned len = ARRAY_SIZE(src_16bit);
    uint16_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_16bit, len,
                                             abs_isub16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.int16_t",
    .start = absoluteDifference_int16,
    .image_filename = "128x128-green.ref.png",
};

static void
absoluteDifference_uint16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 127;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len + int(SUM_N_to_1(len)) - len + 7) / 8];
        };

        uint16_t get_word(uint i)
        {
                return uint16_t(bitfieldExtract(data[i / 8u][(i % 8u) / 2u],
                                                int((i % 2u) * 16u),
                                                16));
        }

        uint16_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return uint16_t(0);

            /* Ironically, max(i, j) - min(i, j) == absoluteDifference(i, j). */
            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return uint16_t(get_word(idx));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint16_t a = get_word(i);
            uint16_t b = get_word(j);
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 127);

    const unsigned len = ARRAY_SIZE(src_16bit);
    uint16_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_16bit, len,
                                             abs_usub16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.uint16_t",
    .start = absoluteDifference_uint16,
    .image_filename = "128x128-green.ref.png",
};

/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 4 bytes.  This results in a maximum of
 * 4096 components.  absoluteDifference is commutative, so instead of
 * explicitly storing N^2 results, we store N+(N-1)+(N-2)+...+1 = (N+1)*N/2
 * results.  The total storage requirement is (N+1)*N/2+N.  N=89 would require
 * 4094 components, and N=90 would require 4185 components.
 *
 * The storage requirement can be further reduced by observing that the
 * diagonal of the result matrix is always 0 because absoluteDifference(x, x)
 * == 0.  The new total storage requirement is (((N+1)*N/2)-N)+N (because the
 * diagonal is N elements).  N=90 would require 4095 components, and N=91
 * would require 4186 components.  One more test vector!  TOTALLY WORTH IT!
 */
static const int32_t src_32bit[] = {
    0x80000000, 0x80000001, 0xe0000000, 0xe0000001, // 4
    0xf8000000, 0xf8000001, 0xfe000000, 0xfe000001, // 8
    0xff800000, 0xff800001, 0xffe00000, 0xffe00001, // 12
    0xfff80000, 0xfff80001, 0xfffe0000, 0xfffe0001, // 16
    0xffff8000, 0xffff8001, 0xffffe000, 0xffffe001, // 20
    0xfffff800, 0xfffff801, 0xfffffe00, 0xfffffe01, // 24
    0xffffff80, 0xffffff81, 0xffffffe0, 0xffffffe1, // 28
    0xfffffff8, 0xfffffff9,                         // 30
    0xfffffffa, 0xfffffffb, 0xfffffffc, 0xfffffffd, // 34
    0xfffffffe, 0xffffffff,                         // 36
    0x00000000,                                     // 37
    0x00000001, 0x00000002, 0x00000003, 0x00000004, // 41
    0x00000005, 0x00000006,                         // 43
    0x00000007, 0x00000008, 0x0000001f, 0x00000020, // 47
    0x0000007f, 0x00000080, 0x000001ff, 0x00000200, // 51
    0x000007ff, 0x00000800, 0x00001fff, 0x00002000, // 55
    0x00007fff, 0x00008000, 0x0001ffff, 0x00020000, // 59
    0x0007ffff, 0x00080000, 0x001fffff, 0x00200000, // 63
    0x007fffff, 0x00800000, 0x01ffffff, 0x02000000, // 67
    0x07ffffff, 0x08000000, 0x1fffffff, 0x20000000, // 71
    0x7fffffff,                                     // 72

    // Some prime numbers requiring from 14- to 32-bits to store.  The last is
    // actually negative.
    0x00002ff9, 0x00003703, 0x0000d159, 0x0000f95f, // 76
    0x00010e17, 0x00013ceb, 0x0001bec3, 0x000b08ed, // 80
    0x0017fff5, 0x0020e1b3, 0x007a2b2b, 0x00ec4ba7, // 84
    0x017ffff5, 0x030a559f, 0x05fffffb,             // 87
    0x2ffffff5, 0x6cbbfe89, 0xbffffff5,             // 90
};

static void
absoluteDifference_int32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len + int(SUM_N_to_1(len)) - len + 3) / 4];
        };


        uint get_expected_result(uint i, uint j)
        {
            if (i == j)
                return 0u;

            /* Ironically, max(i, j) - min(i, j) == absoluteDifference(i, j). */
            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return uint(data[idx / 4][idx % 4]);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int a = data[i / 4][i % 4];
            int b = data[j / 4][j % 4];
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 90);

    const unsigned len = ARRAY_SIZE(src_32bit);
    uint32_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_32bit, len,
                                             abs_isub32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.int",
    .start = absoluteDifference_int32,
    .image_filename = "128x128-green.ref.png",
};

static void
absoluteDifference_uint32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len + int(SUM_N_to_1(len)) - len + 3) / 4];
        };


        uint get_expected_result(uint i, uint j)
        {
            if (i == j)
                return 0u;

            /* Ironically, max(i, j) - min(i, j) == absoluteDifference(i, j). */
            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return data[idx / 4][idx % 4];
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint a = data[i / 4][i % 4];
            uint b = data[j / 4][j % 4];
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 90);

    const unsigned len = ARRAY_SIZE(src_32bit);
    uint32_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_32bit, len,
                                             abs_usub32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.uint",
    .start = absoluteDifference_uint32,
    .image_filename = "128x128-green.ref.png",
};


/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 8 bytes.  This results in a maximum of
 * 2048 components.  absoluteDifference is commutative, so instead of
 * explicitly storing N^2 results, we store N+(N-1)+(N-2)+...+1 = (N+1)*N/2
 * results.  The total storage requirement is (N+1)*N/2+N.  N=62 would require
 * 2015 components, and N=63 would require 2079 components.
 *
 * The storage requirement can be further reduced by observing that the
 * diagonal of the result matrix is always 0 because absoluteDifference(x, x)
 * == 0.  The new total storage requirement is (((N+1)*N/2)-N)+N (because the
 * diagonal is N elements).  N=63 would require 2016 components, and N=64
 * would require 2080 components.  One more test vector!  TOTALLY WORTH IT!
 */
static const int64_t src_64bit[] = {
    0x8000000000000000ll, 0x8000000000000001ll,
    0xfe00000000000000ll, 0xfe00000000000001ll,
    0xfff8000000000000ll, 0xfff8000000000001ll,
    0xffffe00000000000ll, 0xffffe00000000001ll,
    0xffffff8000000000ll, 0xffffff8000000001ll,
    0xfffffffe00000000ll, 0xfffffffe00000001ll,
    0xfffffffff8000000ll, 0xfffffffff8000001ll,
    0xffffffffffe00000ll, 0xffffffffffe00001ll,
    0xffffffffffff8000ll, 0xffffffffffff8001ll,
    0xfffffffffffffe00ll, 0xfffffffffffffe01ll,
    0xfffffffffffffff8ll, 0xfffffffffffffff9ll,
    0xfffffffffffffffbll, 0xfffffffffffffffdll,
    0xfffffffffffffffell, 0xffffffffffffffffll,
    0x0000000000000000ll,
    0x0000000000000001ll, 0x0000000000000002ll,
    0x0000000000000003ll, 0x0000000000000005ll,
    0x0000000000000007ll, 0x0000000000000008ll,
    0x00000000000001ffll, 0x0000000000000200ll,
    0x0000000000007fffll, 0x0000000000008000ll,
    0x00000000001fffffll, 0x0000000000200000ll,
    0x0000000007ffffffll, 0x0000000008000000ll,
    0x00000001ffffffffll, 0x0000000200000000ll,
    0x0000007fffffffffll, 0x0000008000000000ll,
    0x00001fffffffffffll, 0x0000200000000000ll,
    0x0007ffffffffffffll, 0x0008000000000000ll,
    0x01ffffffffffffffll, 0x0200000000000000ll,
    0x7fffffffffffffffll,

    0x000000017ffffffbll, 0x00000017ffffffefll,
    0x0000017ffffffff3ll, 0x000017ffffffffffll,
    0x00017fffffffffe1ll, 0x0005ffffffffffddll,
    0x0017fffffffffff3ll, 0x017fffffffffffb5ll,
    0x17ffffffffffffe1ll, 0x5fffffffffffff89ll,
    0xbfffffffffffffe1ll,
};

static void
absoluteDifference_int64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 63;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            i64vec2 data[(len + int(SUM_N_to_1(len)) - len + 1) / 2];
        };

        uint64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return uint64_t(0);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            i64vec2 d = data[idx / 2];
            return uint64_t((idx & 1) == 0 ? d.x : d.y);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int64_t a = data[i / 2][i % 2];
            int64_t b = data[j / 2][j % 2];
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 63);

    const unsigned len = ARRAY_SIZE(src_64bit);
    uint64_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_64bit, len,
                                             abs_isub64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.int64_t",
    .start = absoluteDifference_int64,
    .image_filename = "64x64-green.ref.png",
};

static void
absoluteDifference_uint64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 63;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            u64vec2 data[(len + int(SUM_N_to_1(len)) - len + 1) / 2];
        };

        uint64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return uint64_t(0);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            u64vec2 d = data[idx / 2];
            return (idx & 1) == 0 ? d.x : d.y;
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint64_t a = data[i / 2][i % 2];
            uint64_t b = data[j / 2][j % 2];
            if (absoluteDifference(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 63);

    const unsigned len = ARRAY_SIZE(src_64bit);
    uint64_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_64bit, len,
                                             abs_usub64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.absoluteDifference.uint64_t",
    .start = absoluteDifference_uint64,
    .image_filename = "64x64-green.ref.png",
};

#define hadd_template(name, type)                                       \
static void                                                             \
name(void *_dest, unsigned dest_index,                                  \
     const void *_src_data, unsigned i, unsigned j)                     \
{                                                                       \
    type *dest = (type *) _dest;                                        \
    const type *src_data = (const type *) _src_data;                    \
    const type a = src_data[i];                                         \
    const type b = src_data[j];                                         \
                                                                        \
    dest[dest_index] = (a >> 1) + (b >> 1) + ((a & b) & 1);             \
}

hadd_template(s_hadd16, int16_t);
hadd_template(u_hadd16, uint16_t);
hadd_template(s_hadd32, int);
hadd_template(u_hadd32, uint);
hadd_template(s_hadd64, int64_t);
hadd_template(u_hadd64, uint64_t);

static void
average_int16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 127;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len + int(SUM_N_to_1(len)) - len + 7) / 8];
        };

        int16_t get_word(uint i)
        {
            return int16_t(bitfieldExtract(data[i / 8u][(i % 8u) / 2u],
                                           int((i % 2u) * 16u),
                                           16));
        }

        int16_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int16_t a = get_word(i);
            int16_t b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 127);

    const unsigned len = ARRAY_SIZE(src_16bit);
    uint16_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_16bit, len,
                                             s_hadd16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.int16_t",
    .start = average_int16,
    .image_filename = "128x128-green.ref.png",
};

static void
average_uint16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 127;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len + int(SUM_N_to_1(len)) - len + 7) / 8];
        };

        uint16_t get_word(uint i)
        {
            return int16_t(bitfieldExtract(data[i / 8u][(i % 8u) / 2u],
                                           int((i % 2u) * 16u),
                                           16));
        }

        uint16_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint16_t a = get_word(i);
            uint16_t b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 127);

    const unsigned len = ARRAY_SIZE(src_16bit);
    uint16_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_16bit, len,
                                             u_hadd16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.uint16_t",
    .start = average_uint16,
    .image_filename = "128x128-green.ref.png",
};

static void
average_int32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len + int(SUM_N_to_1(len)) - len + 3) / 4];
        };

        int get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        int get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int a = get_word(i);
            int b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 90);

    const unsigned len = ARRAY_SIZE(src_32bit);
    int32_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_32bit, len,
                                             s_hadd32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.int",
    .start = average_int32,
    .image_filename = "128x128-green.ref.png",
};

static void
average_uint32(void)
{
        const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len + int(SUM_N_to_1(len)) - len + 3) / 4];
        };

        uint get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        uint get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint a = get_word(i);
            uint b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 90);

    const unsigned len = ARRAY_SIZE(src_32bit);
    uint32_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_32bit, len,
                                             u_hadd32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.uint",
    .start = average_uint32,
    .image_filename = "128x128-green.ref.png",
};

static void
average_int64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 63;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            i64vec2 data[(len + int(SUM_N_to_1(len)) - len + 1) / 2];
        };

        int64_t get_word(uint i)
        {
            return data[i / 2u][i % 2u];
        }

        int64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int64_t a = get_word(i);
            int64_t b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 63);

    const unsigned len = ARRAY_SIZE(src_64bit);
    int64_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_64bit, len,
                                             s_hadd64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.int64_t",
    .start = average_int64,
    .image_filename = "64x64-green.ref.png",
};

static void
average_uint64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        QO_DEFINE SUM_N_to_1(x)  (((uint(x)+1u)*uint(x))/2u)
        const int len = 63;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            u64vec2 data[(len + int(SUM_N_to_1(len)) - len + 1) / 2];
        };

        uint64_t get_word(uint i)
        {
            return data[i / 2u][i % 2u];
        }

        uint64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return get_word(i);

            uint row = min(i, j);
            uint col = max(i, j) - row;

            uint k = uint(len - 1) - row;
            uint idx = len + SUM_N_to_1(len - 1) - SUM_N_to_1(k) + (col - 1u);

            return get_word(idx);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint64_t a = get_word(i);
            uint64_t b = get_word(j);
            if (average(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 63);

    const unsigned len = ARRAY_SIZE(src_64bit);
    uint64_t expected[SUM_N_to_1(len) - len];

    generate_results_commutative_no_diagonal(expected, src_64bit, len,
                                             u_hadd64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
                                expected, sizeof(expected));
}

test_define {
    .name = "func.shader.average.uint64_t",
    .start = average_uint64,
    .image_filename = "64x64-green.ref.png",
};
