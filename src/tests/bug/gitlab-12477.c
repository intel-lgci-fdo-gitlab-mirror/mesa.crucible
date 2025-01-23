// Copyright 2025 Intel Corporation
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

/* This is a test for https://gitlab.freedesktop.org/mesa/mesa/-/issues/12477
 *
 * The issue is an incorrect CSE optimization applies to float values followed
 * by a bcsel.
 */

#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-12477-spirv.h"

struct ssbo_data {
    uint32_t condition;
    float input_value;
    float output_value;
};

static void
test(void)
{
    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
        layout(set = 0, binding = 0, std430) buffer block2 {
            uint condition;
            float input_value;
            float output_value;
        } ssbo;

        layout (local_size_x = 1) in;
        void main()
        {
            float v1 = ssbo.input_value * 0.5;
            float v2 = ssbo.input_value * -0.5;
            ssbo.output_value = ssbo.condition != 0 ? v1 : v2;
        }
    );

    struct ssbo_data ssbo_data = {
        .condition = 0,
        .input_value = 42.0,
        .output_value = 43.0,
    };
    simple_compute_pipeline_options_t opts = {
        .storage = &ssbo_data,
        .storage_size = sizeof(ssbo_data),
    };
    run_simple_compute_pipeline(cs, &opts);

    t_assert(ssbo_data.output_value == -21.0);

    t_pass();
}

test_define {
    .name = "bug.gitlab.12477",
    .start = test,
    .no_image = true,
};
