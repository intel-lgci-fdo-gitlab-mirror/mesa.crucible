// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

#include "src/tests/bug/gitlab-12927-spirv.h"

// \file
// Reproduce an Intel compiler bug from mesa#12927.
//
// The problem happens with the index passed to subgroupShuffle() is
// convergent.

struct test_params {
    unsigned lane;
};


struct ssbo_data {
    uint32_t lane;
    uint32_t subgroup_size;
    uint32_t input_values[32];
    uint32_t output_values[32];
};

static void
test(void)
{
    const struct test_params *params = t_user_data;

    VkShaderModule cs = qoCreateShaderModuleGLSL(t_device, COMPUTE,
        QO_TARGET_ENV spirv1.4
        QO_EXTENSION GL_KHR_shader_subgroup_basic: require
        QO_EXTENSION GL_KHR_shader_subgroup_shuffle: require
        layout(binding = 0, std430) buffer block2 {
            uint lane;
            uint subgroup_size;
            uint input_values[32];
            uint output_values[32];
        } ssbo;

        layout (local_size_x = 32) in;
        void main()
        {
            ssbo.subgroup_size = gl_SubgroupSize;
            uint value = ssbo.input_values[gl_SubgroupInvocationID];
            ssbo.output_values[gl_SubgroupID * gl_SubgroupSize +
                               gl_SubgroupInvocationID] =
                subgroupShuffle(value, ssbo.lane);
        }
    );

    struct ssbo_data ssbo_data = {
        .lane          = params->lane,
        .subgroup_size = 0,
    };
    for (unsigned i = 0; i < 32; i++)
        ssbo_data.input_values[i] = i;

    simple_compute_pipeline_options_t opts = {
        .storage = &ssbo_data,
        .storage_size = sizeof(ssbo_data),
        .required_subgroup_size = 32,
    };

    run_simple_compute_pipeline(cs, &opts);

    t_assert(ssbo_data.subgroup_size == 32);
    for (unsigned i = 0; i < 32; i++)
        t_assert(ssbo_data.output_values[i] == params->lane);

    t_pass();
}

test_define {
    .name = "bug.gitlab-12927.lane0",
    .start = test,
    .user_data = &(struct test_params) { .lane = 0, },
    .no_image = true,
};

test_define {
    .name = "bug.gitlab-12927.lane3",
    .start = test,
    .user_data = &(struct test_params) { .lane = 3, },
    .no_image = true,
};

test_define {
    .name = "bug.gitlab-12927.lane17",
    .start = test,
    .user_data = &(struct test_params) { .lane = 17, },
    .no_image = true,
};

test_define {
    .name = "bug.gitlab-12927.lane30",
    .start = test,
    .user_data = &(struct test_params) { .lane = 30, },
    .no_image = true,
};
