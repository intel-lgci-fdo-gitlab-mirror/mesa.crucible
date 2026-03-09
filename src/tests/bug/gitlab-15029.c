// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-15029-spirv.h"

// \file
// Reproduce an Intel anv bug from mesa#15029.
//
// The problem is a missing base offset indexing in pulling push
// constant data.

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))

static void
test(void)
{
    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
        QO_TARGET_ENV spirv1.4
        layout(push_constant, std430) uniform Push {
            uint pad[8];
            uint input_vals[8];
        } push;
        layout(binding = 0, std430) buffer block2 {
            uint output_val;
        } ssbo;

        layout (local_size_x = 1) in;
        void main()
        {
            uint sum = 0;
            for (uint i = 0; i < 8; i++)
                sum += push.input_vals[i];
            ssbo.output_val = sum;
        }
    );

    uint32_t push[16];
    uint32_t result = 0;

    uint32_t sum = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(push); i++) {
      push[i] = i;
      sum += i >= 8 ? i : 0;
    }

    simple_compute_pipeline_options_t opts = {
        .push_constants = push,
        .push_constants_size = sizeof(push),
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_compute_pipeline(cs, &opts);

    t_assert(result == sum);

    t_pass();
}

test_define {
    .name = "bug.gitlab-15029",
    .start = test,
    .no_image = true,
};
