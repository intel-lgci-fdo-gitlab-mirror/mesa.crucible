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

#include "tapi/t.h"
#include "intel_shader_integer_functions2_common.h"

#include "src/tests/func/intel_shader_integer_functions2/subtractSaturate-spirv.h"

#define isub_sat_template(name, type, min, max)                         \
static void                                                             \
name(void *_dest, unsigned dest_index,                                  \
     const void *_src_data, unsigned i, unsigned j)                     \
{                                                                       \
    type *dest = (type *) _dest;                                        \
    const type *src_data = (const type *) _src_data;                    \
    const type a = src_data[i];                                         \
    const type b = src_data[j];                                         \
                                                                        \
    if (a >= 0) {                                                       \
        if ((a - max) > b) {                                            \
            dest[dest_index] = max;                                     \
            return;                                                     \
        }                                                               \
    } else if (b > 0) {                                                 \
        if (a < (min + b)) {                                            \
            dest[dest_index] = min;                                     \
            return;                                                     \
        }                                                               \
    }                                                                   \
    dest[dest_index] = a - b;                                           \
}

isub_sat_template(isub_sat16, int16_t, INT16_MIN, INT16_MAX);
isub_sat_template(isub_sat32, int32_t, INT32_MIN, INT32_MAX);
isub_sat_template(isub_sat64, int64_t, INT64_MIN, INT64_MAX);

#define usub_sat_template(name, type)                                   \
static void                                                             \
name(void *_dest, unsigned dest_index,                                  \
     const void *_src_data, unsigned i, unsigned j)                     \
{                                                                       \
    type *dest = (type *) _dest;                                        \
    const type *src_data = (const type *) _src_data;                    \
    const type a = src_data[i];                                         \
    const type b = src_data[j];                                         \
                                                                        \
    dest[dest_index] = b > a ? 0 : a - b;                               \
}

usub_sat_template(usub_sat16, uint16_t);
usub_sat_template(usub_sat32, uint32_t);
usub_sat_template(usub_sat64, uint64_t);

/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 2 bytes.  This results in a maximum of
 * 8192 components.  subtractSaturate is not commutative, so the full set of
 * N^2 results must be stored.  The total storate requirement is (N*N)+N.
 * N=90 would require 8190 components, and N=91 would require 8372 components.
 *
 * The storage requirement can be reduced by observing that the diagonal of
 * the result matrix is always 0 because subtractSaturate(x, x) == 0.  The new
 * total storage requirement is ((N-1)*N)+N = N^2.  N=90 would require 8100
 * components, and N=91 would require 8281 components.  BOO!
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
    0x07f1, 0x07f2, 0x07f3,                                         // 90
};

static void
subtractSaturate_int16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len * len + 7) / 8];
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
                return int16_t(0);

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int16_t a = get_word(i);
            int16_t b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 90);

    const unsigned len = ARRAY_SIZE(src_16bit);
    int16_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_16bit, len, isub_sat16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.int16_t",
    .start = subtractSaturate_int16,
    .image_filename = "128x128-green.ref.png",
};

static void
subtractSaturate_uint16(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt16)
        t_skipf("shaderInt16 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int16: require

        const int len = 90;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len * len + 7) / 8];
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

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint16_t a = get_word(i);
            uint16_t b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_16bit) == 90);

    const unsigned len = ARRAY_SIZE(src_16bit);
    uint16_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_16bit, len, usub_sat16);
    run_integer_functions2_test(&fs_info, src_16bit, sizeof(src_16bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.uint16_t",
    .start = subtractSaturate_uint16,
    .image_filename = "128x128-green.ref.png",
};

/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 4 bytes.  This results in a maximum of
 * 4096 components.  subtractSaturate is not commutative, so the full set of
 * N^2 results must be stored.  The total storate requirement is (N*N)+N.
 * N=63 would require 4032 components, and N=64 would require 4160 components.
 *
 * The storage requirement can be reduced by observing that the diagonal of
 * the result matrix is always 0 because subtractSaturate(x, x) == 0.  The new
 * total storage requirement is ((N-1)*N)+N = N^2.  N=64 would require 4096
 * components exactly.  One more test vector!  TOTALLY WORTH IT!
 */
static const int32_t src_32bit[] = {
    0x80000000, 0x80000001, 0xf8000000, 0xf8000001, // 4
    0xff800000, 0xff800001, 0xfff80000, 0xfff80001, // 8
    0xffff8000, 0xffff8001, 0xfffff800, 0xfffff801, // 12
    0xffffff80, 0xffffff81, 0xfffffff8, 0xfffffff9, // 16
    0xfffffffa, 0xfffffffb, 0xfffffffe, 0xffffffff, // 20
    0x00000000,                                     // 21
    0x00000001, 0x00000002, 0x00000003, 0x00000004, // 25
    0x00000005, 0x00000006,                         // 27
    0x00000007, 0x00000008, 0x0000007f, 0x00000080, // 31
    0x000007ff, 0x00000800, 0x00007fff, 0x00008000, // 35
    0x0007ffff, 0x00080000, 0x007fffff, 0x00800000, // 39
    0x07ffffff, 0x08000000, 0x7fffffff,             // 42

    // Some prime numbers requiring from 11- to 32-bits to store.  The last is
    // actually negative.
    0x000007f7, 0x00000ffd, 0x00001fff, 0x00002ff9, // 46
    0x00003703, 0x0000d159, 0x0000f95f, 0x00010e17, // 50
    0x00013ceb, 0x0001bec3, 0x000b08ed, 0x0017fff5, // 54
    0x0020e1b3, 0x007a2b2b, 0x00ec4ba7, 0x017ffff5, // 58
    0x030a559f, 0x05fffffb, 0x0ab1cda1, 0x2ffffff5, // 62
    0x6cbbfe89, 0xbffffff5,                         // 64
};

static void
subtractSaturate_int32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        const int len = 64;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[(len * len + 3) / 4];
        };

        int get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        int get_expected_result(uint i, uint j)
        {
            if (i == j)
                return 0;

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int a = get_word(i);
            int b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 64);

    const unsigned len = ARRAY_SIZE(src_32bit);
    int32_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_32bit, len, isub_sat32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.int",
    .start = subtractSaturate_int32,
    .image_filename = "64x64-green.ref.png",
};

static void
subtractSaturate_uint32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        const int len = 64;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[(len * len + 3) / 4];
        };

        uint get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        uint get_expected_result(uint i, uint j)
        {
            if (i == j)
                return 0u;

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint a = get_word(i);
            uint b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == 64);

    const unsigned len = ARRAY_SIZE(src_32bit);
    int32_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_32bit, len, usub_sat32);
    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.uint",
    .start = subtractSaturate_uint32,
    .image_filename = "64x64-green.ref.png",
};

/* Vulkan 1.0 requires that implementations support uniform buffers of at
 * least 16384 bytes, and each value is 8 bytes.  This results in a maximum of
 * 2048 components.  subtractSaturate is not commutative, so the full set of
 * N^2 results must be stored.  The total storate requirement is (N*N)+N.
 * N=44 would require 1980 components, and N=45 would require 2070 components.
 *
 * The storage requirement can be reduced by observing that the diagonal of
 * the result matrix is always 0 because subtractSaturate(x, x) == 0.  The new
 * total storage requirement is ((N-1)*N)+N = N^2.  N=45 would require 2025
 * components, and N=46 would require 2116 components.  One more test vector!
 * TOTALLY WORTH IT!
 */
static const int64_t src_64bit[] = {
    0x8000000000000000ll, 0x8000000000000001ll,
    0xfe00000000000000ll, 0xfe00000000000001ll,
    0xfff8000000000000ll, 0xfff8000000000001ll,
    0xffffe00000000000ll, 0xffffe00000000001ll,
    0xfffffffe00000000ll, 0xfffffffe00000001ll,
    0xffffffffffe00000ll, 0xffffffffffe00001ll,
    0xffffffffffff8000ll, 0xffffffffffff8001ll,
    0xfffffffffffffe00ll, 0xfffffffffffffe01ll,
    0xfffffffffffffff8ll, 0xfffffffffffffff9ll,
    0xfffffffffffffffbll, 0xfffffffffffffffdll,
    0xfffffffffffffffell, 0xffffffffffffffffll,
    0x0000000000000001ll, 0x0000000000000002ll,
    0x0000000000000003ll, 0x0000000000000005ll,
    0x0000000000000007ll, 0x0000000000000008ll,
    0x00000000000001ffll, 0x0000000000000200ll,
    0x0000000000007fffll, 0x0000000000008000ll,
    0x00000000001fffffll, 0x0000000000200000ll,
    0x00000001ffffffffll, 0x0000000200000000ll,
    0x0000007fffffffffll, 0x0000008000000000ll,
    0x0007ffffffffffffll, 0x0008000000000000ll,
    0x01ffffffffffffffll, 0x0200000000000000ll,
    0x7fffffffffffffffll, 0x0000000000000000ll,
    0xf0f0f0f0f0f0f0f0ll,
};

static void
subtractSaturate_int64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        const int len = 45;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            i64vec2 data[(len * len) + 1 / 2];
        };

        int64_t get_word(uint i)
        {
            i64vec2 v = data[i / 2u];
            return (i & 1) == 0 ? v.x : v.y;
        }

        int64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return int64_t(0);

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            int64_t a = get_word(i);
            int64_t b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 45);

    const unsigned len = ARRAY_SIZE(src_64bit);
    int64_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_64bit, len, isub_sat64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.int64_t",
    .start = subtractSaturate_int64,
    .image_filename = "64x64-green.ref.png",
};

static void
subtractSaturate_uint64(void)
{
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(t_physical_dev, &features);
    if (!features.shaderInt64)
        t_skipf("shaderInt64 not supported");

    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require
        QO_EXTENSION GL_ARB_gpu_shader_int64: require

        const int len = 45;

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            u64vec2 data[(len * len) + 1 / 2];
        };

        uint64_t get_word(uint i)
        {
            u64vec2 v = data[i / 2u];
            return (i & 1) == 0 ? v.x : v.y;
        }

        uint64_t get_expected_result(uint i, uint j)
        {
            if (i == j)
                return uint64_t(0);

            /* The first len elements are the source data, so skip those.  If
             * j > i, j is on the other side of the (omitted) diagonal, so the
             * offset must be decremented.
             */
            return get_word(len + (i * (len - 1)) + j - int(j > i));
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % uint(len);
            uint j = uint(gl_FragCoord.y) % uint(len);

            uint64_t a = get_word(i);
            uint64_t b = get_word(j);
            if (subtractSaturate(a, b) == get_expected_result(i, j))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_64bit) == 45);

    const unsigned len = ARRAY_SIZE(src_64bit);
    uint64_t expected[(len * len) - len];

    generate_results_no_diagonal(expected, src_64bit, 45, usub_sat64);
    run_integer_functions2_test(&fs_info, src_64bit, sizeof(src_64bit),
				expected, sizeof(expected));
}

test_define {
    .name = "func.shader.subtractSaturate.uint64_t",
    .start = subtractSaturate_uint64,
    .image_filename = "64x64-green.ref.png",
};
