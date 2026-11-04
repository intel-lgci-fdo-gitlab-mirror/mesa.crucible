// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "tapi/t.h"
#include "util/simple_pipeline.h"

#include "src/tests/func/compute/struct-phi-spvasm.h"

static void
test(void)
{
    const uint32_t condition = *(const uint32_t *)t_user_data;
    uint32_t data[] = { condition, 0, 0 };

    VkShaderModule cs = qoCreateShaderModuleSPIRVASM(t_device,
        ; QO_TARGET_ENV vulkan1.0
        OpCapability Shader
        OpMemoryModel Logical GLSL450
        OpEntryPoint GLCompute %main "main"
        OpExecutionMode %main LocalSize 1 1 1

        OpDecorate %Block BufferBlock
        OpMemberDecorate %Block 0 Offset 0
        OpMemberDecorate %Block 1 Offset 4
        OpMemberDecorate %Block 2 Offset 8
        OpDecorate %data DescriptorSet 0
        OpDecorate %data Binding 0

        %void = OpTypeVoid
        %bool = OpTypeBool
        %uint = OpTypeInt 32 0
        %uint0 = OpConstant %uint 0
        %uint1 = OpConstant %uint 1
        %uint2 = OpConstant %uint 2
        %uint3 = OpConstant %uint 3
        %uint4 = OpConstant %uint 4
        %Pair = OpTypeStruct %uint %uint
        %pair_a = OpConstantComposite %Pair %uint1 %uint2
        %pair_b = OpConstantComposite %Pair %uint3 %uint4
        %Block = OpTypeStruct %uint %uint %uint
        %ptr_Uniform_Block = OpTypePointer Uniform %Block
        %ptr_Uniform_uint = OpTypePointer Uniform %uint
        %data = OpVariable %ptr_Uniform_Block Uniform
        %func = OpTypeFunction %void

        %main = OpFunction %void None %func
        %entry = OpLabel
        %condition_ptr = OpAccessChain %ptr_Uniform_uint %data %uint0
        %condition = OpLoad %uint %condition_ptr
        %choose_a = OpINotEqual %bool %condition %uint0
        OpSelectionMerge %merge None
        OpBranchConditional %choose_a %then %else

        %then = OpLabel
        OpBranch %merge

        %else = OpLabel
        OpBranch %merge

        %merge = OpLabel
        %pair = OpPhi %Pair %pair_a %then %pair_b %else
        %first = OpCompositeExtract %uint %pair 0
        %second = OpCompositeExtract %uint %pair 1
        %first_ptr = OpAccessChain %ptr_Uniform_uint %data %uint1
        %second_ptr = OpAccessChain %ptr_Uniform_uint %data %uint2
        OpStore %first_ptr %first
        OpStore %second_ptr %second
        OpReturn
        OpFunctionEnd
    );

    simple_compute_pipeline_options_t opts = {
        .storage = data,
        .storage_size = sizeof(data),
    };
    run_simple_compute_pipeline(cs, &opts);

    const uint32_t expected[] = { condition ? 1 : 3, condition ? 2 : 4 };
    t_assertf(data[1] == expected[0] && data[2] == expected[1],
              "got {%u, %u}, expected {%u, %u}", data[1], data[2],
              expected[0], expected[1]);
    t_pass();
}

test_define {
    .name = "func.compute.struct-phi.false",
    .start = test,
    .user_data = &(const uint32_t) { 0 },
    .no_image = true,
    .queue_setup = QUEUE_SETUP_COMPUTE,
};

test_define {
    .name = "func.compute.struct-phi.true",
    .start = test,
    .user_data = &(const uint32_t) { 1 },
    .no_image = true,
    .queue_setup = QUEUE_SETUP_COMPUTE,
};
