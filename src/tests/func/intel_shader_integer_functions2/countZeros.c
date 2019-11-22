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

#include "src/tests/func/intel_shader_integer_functions2/countZeros-spirv.h"

static void
countLeadingZeros_uint32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        QO_DEFINE src_length 4096

        layout(set = 0, binding = 0) uniform Data {
            uvec4 data[(src_length + 3) / 4];
        };

        layout(location = 0) out vec4 f_color;

        void main()
        {
            const uint l = uint(src_length);
            uint i = uint(gl_FragCoord.x) % 64u;
            uint j = uint(gl_FragCoord.y) % 64u;
            uint idx = (i * 64u) | j;

            uint value = data[idx / 4u][idx % 4u];
            uint expected = idx != (src_length - 1) ? idx % 32u : 32u;


            if (countLeadingZeros(value) == expected)
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    uint32_t src[4096];

    /* Generate source data so that the expected results follow a sequence 0,
     * 1, 2, 3, ... 30, 31, 0, 1, ...
     */
    for (unsigned i = 0; i < 4095; i++) {
        unsigned bit = i % 32u;
        unsigned mask = (0x80000000u >> bit) - 1;

        src[i] = (0x80000000 >> bit) | (rand() & mask);
    }

    src[4095] = 0;

    run_integer_functions2_test(&fs_info, src, sizeof(src), NULL, 0);
}

test_define {
    .name = "func.shader.countLeadingZeros.uint",
    .start = countLeadingZeros_uint32,
    .image_filename = "64x64-green.ref.png",
};

static void
countTrailingZeros_uint32(void)
{
    const QoShaderModuleCreateInfo fs_info = qoShaderModuleCreateInfoGLSL(FRAGMENT,
        QO_EXTENSION GL_INTEL_shader_integer_functions2: require

        const uint len = 4096u;

        layout(set = 0, binding = 0) uniform Data {
            uvec4 data[(len + 3u) / 4u];
        };

        layout(location = 0) out vec4 f_color;

        void main()
        {
            uint i = uint(gl_FragCoord.x) % 64u;
            uint j = uint(gl_FragCoord.y) % 64u;
            uint idx = (i * 64u) | j;

            uint value = data[idx / 4u][idx % 4u];
            uint expected = idx != (len - 1) ? idx % 32u : 32u;

            if (countTrailingZeros(value) == expected)
                f_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                f_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    uint32_t src[4096];

    /* Generate source data so that the expected results follow a sequence 0,
     * 1, 2, 3, ... 30, 31, 0, 1, ...
     */
    for (unsigned i = 0; i < 4095; i++) {
        unsigned bit = i % 32u;
        unsigned mask = ~((1 << bit) - 1);

        src[i] = (1u << bit) | (rand() & mask);
    }

    src[4095] = 0;

    run_integer_functions2_test(&fs_info, src, sizeof(src), NULL, 0);
}

test_define {
    .name = "func.shader.countTrailingZeros.uint",
    .start = countTrailingZeros_uint32,
    .image_filename = "64x64-green.ref.png",
};
