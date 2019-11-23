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

#include "src/tests/func/intel_shader_integer_functions2/multiply32x16-spirv.h"

static const int32_t src_32bit[] = {
    0x00004b09, 0x00007c35, 0x00000c02, 0x00007dac,
    0x00000cad, 0xffffe711, 0xffff912c, 0xffff96b9,
    0xffffb5b7, 0x00007e1f, 0x0000343c, 0xffff9f6c,
    0x000011db, 0x000007f1, 0xffffb9a9, 0xffffd080,
    0x095085be, 0x7a1ad760, 0xf27cab94, 0x62cae2ff,
    0xe41bbffe, 0x89d3d59d, 0x45042125, 0xf993a46e,
    0x920c3247, 0x7366bc7c, 0x5ea1d2b0, 0xa08886aa,
    0xde6dca6e, 0xc8ecaf5e, 0x386d0b65, 0x67f02c35,
    0x20741c20, 0x1c2667ec, 0x188f9081, 0xd0514f84,
    0x61816672, 0xd59129d7, 0x2fc1d222, 0xdbdd333a,
    0xf87c2bd9, 0x64654972, 0x29d8f285, 0x4665cffd,
    0x11f5f1da, 0x66de0c63, 0x7b976cda, 0x8f210d8f,
    0x7e3931c2, 0x8311b5b8, 0x36c1c5fc, 0x66a1fcd5,
    0xe1959881, 0xfcfc27e9, 0x0a434057, 0x75366d48,
    0xc95548fd, 0x3750305d, 0xe5b277bb, 0xd63cd0e5,
    0xb4169d42, 0x8eb3b61a, 0x10ebcb18, 0x188ec853,
    0xd357fc61, 0x168408d1, 0x739f2f57, 0xa6453eb5,
    0xdf803b08, 0xee9d7207, 0x41343447, 0x15679e50,
    0x12fa03b6, 0x763bcaba, 0xcd6319a2, 0x36577151,
    0xf5ed6382, 0x3dc688b6, 0x6f51eb99, 0x62285c37,
    0x23063c00, 0x6a31fde7, 0xe974d850, 0xf45dc73a,
    0xf2fdae72, 0xabe9e67e, 0x099e9e43, 0xd96b3b85,
    0x3f595a74, 0x92aec106, 0x16a093df, 0xa3879a3b,
    0x5cecce1b, 0x4a98eebc, 0xe8f65d68, 0x7fb7bbbb,
    0xc98279cf, 0x109c5a91, 0xee261274, 0xf64fff46,
    0xf785bb80, 0x0cfaef83, 0x3db2e45b, 0xe27fd7f6,
    0x909227bd, 0x01a30184, 0x6331c303, 0xab49e15b,
    0x969d6eda, 0xe7e2567f, 0x18e7ded7, 0x0e678bf5,
    0x0d69bc10, 0x055c8aab, 0x14a3234b, 0x0f282b90,
    0x111ca924, 0xaa63623d, 0x7650e2ad, 0xd4917505,
    0x479c35fd, 0x11546572, 0xe018dc09, 0x7862003a,
    0x4bfa1d06, 0xb40a9b48, 0xdad612cc, 0xa3e79956,

    /* offset and number of bits for the bitfieldExtract call. */
    0, 16,
};

static void
multiply32x16_int32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            ivec4 data[128 / 4];
            ivec2 field;
        };

        int get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        int fake_multiply32x16(int x, int y)
        {
            /* The bitfieldExtract nonsense is to prevent clever compilers
             * from turning this multiply into a multiply32x16.  Such a
             * (reasonable!) optimization would make the test pointless.
             */
            return x * bitfieldExtract(y, field.x, field.y);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % 128u;
            uint j = uint(gl_FragCoord.y) % 128u;

            int a = get_word(i);
            int b = get_word(j);
            if (multiply32x16(a, b) == fake_multiply32x16(a, b))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == (128 + 2));

    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit), NULL, 0);
}

test_define {
    .name = "func.shader.multiply32x16.int",
    .start = multiply32x16_int32,
    .image_filename = "128x128-green.ref.png",
};

static void
multiply32x16_uint32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        layout(set = 0, binding = 0, std140) uniform Data {
            /* Store everything in one array to avoid any unused components. */
            uvec4 data[128 / 4];
            ivec2 field;
        };

        uint get_word(uint i)
        {
            return data[i / 4u][i % 4u];
        }

        uint fake_multiply32x16(uint x, uint y)
        {
            /* The bitfieldExtract nonsense is to prevent clever compilers
             * from turning this multiply into a multiply32x16.  Such a
             * (reasonable!) optimization would make the test pointless.
             */
            return x * bitfieldExtract(y, field.x, field.y);
        }

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % 128u;
            uint j = uint(gl_FragCoord.y) % 128u;

            uint a = get_word(i);
            uint b = get_word(j);
            if (multiply32x16(a, b) == fake_multiply32x16(a, b))
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    // This ensures that there are the correct number of data elements listed
    // in the array definition.
    assert(ARRAY_SIZE(src_32bit) == (128 + 2));

    run_integer_functions2_test(&fs_info, src_32bit, sizeof(src_32bit),	NULL, 0);
}

test_define {
    .name = "func.shader.multiply32x16.uint",
    .start = multiply32x16_uint32,
    .image_filename = "128x128-green.ref.png",
};
