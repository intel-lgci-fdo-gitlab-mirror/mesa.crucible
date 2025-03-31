// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-12888-spirv.h"

// \file
// Reproduce an Intel compiler bug from mesa#12888.

struct ssbo_data {
    float in_data[5];
    float out_data[5];
    uint32_t index;
    uint32_t count[64];
};

static void
test(void)
{
    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
        layout(binding = 0, std430) buffer block2 {
            float in_data[5];
            float out_data[5];
            uint index;
            uint count[];
        } ssbo;

        layout (local_size_x = 5) in;
        void main()
        {
            atomicAdd(ssbo.count[ssbo.index], 1);

            uint index = gl_LocalInvocationIndex;

            /* These next few lines look a bit like open-coded sign(). It is
             * possible that optimization passes will convert this to
             * sign(). The problem occurs in the Intel compiler when that
             * optimization occurs late in compilation when sign()
             * instructions are not expected to exist. This can occur, per
             * mesa#12888, when 64-bit integer address arithmetic is lowered.
             *
             * The sign() instruction is unknown to the backend. In the
             * current code, that case falls through to the case for
             * trunc(). Input X coordinate is chosen so that
             * trunc(ssbo.in_data[index]) would not produce the same value as
             * sign(ssbo.in_data[index]).
             *
             * The goofiness with the atomicAdd (above) is necessary to end up
             * with this int64 address arithmetic on some Intel platforms.
             */
            float x = float(ssbo.in_data[index] > 0.0) / 2.0;
            float y = float(0.0 > ssbo.in_data[index]) / 2.0;

            ssbo.out_data[index] = x - y + 0.5;
        }
    );

    struct ssbo_data ssbo_data = {
        { -59.47, 34.56, 0.0, NAN, -NAN },
        { 99.99, 99.99, 99.99, 99.99, 99.99 }
    };

    simple_compute_pipeline_options_t opts = {
        .storage = &ssbo_data,
        .storage_size = sizeof(ssbo_data),
    };

    run_simple_compute_pipeline(cs, &opts);

    t_assert(ssbo_data.out_data[0] == 0.0);
    t_assert(ssbo_data.out_data[1] == 1.0);
    t_assert(ssbo_data.out_data[2] == 0.5);
    t_assert(ssbo_data.out_data[3] == 0.5);
    t_assert(ssbo_data.out_data[4] == 0.5);

    t_pass();
}

test_define {
    .name = "bug.gitlab-12888",
    .start = test,
    .no_image = true,
};
