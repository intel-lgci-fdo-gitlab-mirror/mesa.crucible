// Copyright 2021 Intel Corporation
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

#include "src/tests/func/mesh/nv/task-memory-spirv.h"

/* Task memory flows from each Task workgroup to all its "children"
 * Mesh workgroups.  That implies ordering between their execution.
 *
 * The specification doesn't mandate ordering between different Task
 * workgroups and their children: e.g. it is possible that a Mesh
 * workgroup from the first Task starts before the second Task starts.
 */

static void
task_memory_uint(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint a;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.a = 0x60;
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint a;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[1];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 2) {
                result[0] = taskIn.a;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC,
    };

    uint32_t expected[] = {
        0x60,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.uint",
    .start = task_memory_uint,
    .no_image = true,
};


static void
task_memory_uint64(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int64: enable
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint64_t a;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 13) {
                taskOut.a = (uint64_t(0x60616263) << 32U) + uint64_t(0x64656667);
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        QO_EXTENSION GL_EXT_shader_explicit_arithmetic_types_int64: enable
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint64_t a;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[2];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 2) {
                result[0] = uint((taskIn.a >> 32U) & 0xFFFFFFFFU);
                result[1] = uint(taskIn.a & 0xFFFFFFFFU);
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[]   = { 0xCCCCCCCC, 0xCCCCCCCC };
    uint32_t expected[] = { 0x60616263, 0x64656667 };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.uint64",
    .start = task_memory_uint64,
    .no_image = true,
};

static void
task_memory_uvec4(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uvec4 a;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.a = uvec4(0x60, 0x61, 0x62, 0x63);
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uvec4 a;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[4];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 2) {
                result[0] = taskIn.a.x;
                result[1] = taskIn.a.y;
                result[2] = taskIn.a.z;
                result[3] = taskIn.a.w;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.uvec4",
    .start = task_memory_uvec4,
    .no_image = true,
};

static void
task_memory_array_direct(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint v[8];
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.v[0] = 0x60;
                taskOut.v[1] = 0x61;
                taskOut.v[2] = 0x62;
            } else if (index == 14) {
                taskOut.v[3] = 0x63;
                taskOut.v[4] = 0x64;
                taskOut.v[5] = 0x65;
            } else if (index == 2) {
                taskOut.v[6] = 0x66;
                taskOut.v[7] = 0x67;
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint v[8];
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[8];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 23) {
                result[0] = taskIn.v[0];
                result[1] = taskIn.v[1];
                result[2] = taskIn.v[2];
            } else if (index == 8) {
                result[3] = taskIn.v[3];
                result[4] = taskIn.v[4];
                result[5] = taskIn.v[5];
            } else if (index == 7) {
                result[6] = taskIn.v[6];
                result[7] = taskIn.v[7];
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.array_direct",
    .start = task_memory_array_direct,
    .no_image = true,
};


static void
task_memory_array_indirect(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint v[32];
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            taskOut.v[index] = index;
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint v[32];
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[32];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            result[index] = taskIn.v[index] + 0x60;

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.array_indirect",
    .start = task_memory_array_indirect,
    .no_image = true,
};

static void
task_memory_many(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.a = 0x60;
            } else if (index == 21) {
                taskOut.b = uvec2(0x61, 0x62);
            } else if (index == 12) {
                taskOut.c = uvec3(0x63, 0x64, 0x65);
            } else if (index == 2) {
                taskOut.d = uvec4(0x66, 0x67, 0x68, 0x69);
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[10];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 31) {
                result[0] = taskIn.a;
            } else if (index == 20) {
                result[1] = taskIn.b.x;
                result[2] = taskIn.b.y;
            } else if (index == 8) {
                result[3] = taskIn.c.x;
                result[4] = taskIn.c.y;
                result[5] = taskIn.c.z;
            } else if (index == 7) {
                result[6] = taskIn.d.x;
                result[7] = taskIn.d.y;
                result[8] = taskIn.d.z;
                result[9] = taskIn.d.w;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.many",
    .start = task_memory_many,
    .no_image = true,
};

static void
task_memory_struct(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        struct S {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        };

        taskNV out Task {
            S s;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.s.a = 0x60;
            } else if (index == 21) {
                taskOut.s.b = uvec2(0x61, 0x62);
            } else if (index == 12) {
                taskOut.s.c = uvec3(0x63, 0x64, 0x65);
            } else if (index == 2) {
                taskOut.s.d = uvec4(0x66, 0x67, 0x68, 0x69);
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        struct S {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        };

        taskNV in Task {
            S s;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[10];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 31) {
                result[0] = taskIn.s.a;
            } else if (index == 20) {
                result[1] = taskIn.s.b.x;
                result[2] = taskIn.s.b.y;
            } else if (index == 8) {
                result[3] = taskIn.s.c.x;
                result[4] = taskIn.s.c.y;
                result[5] = taskIn.s.c.z;
            } else if (index == 7) {
                result[6] = taskIn.s.d.x;
                result[7] = taskIn.s.d.y;
                result[8] = taskIn.s.d.z;
                result[9] = taskIn.s.d.w;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.struct",
    .start = task_memory_struct,
    .no_image = true,
};

static void
task_memory_some_not_used(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint a;
            uvec2 b;
            uvec3 not_used;
            uvec3 c;
            uvec4 d;
            uint also_not_used;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.a = 0x60;
            } else if (index == 21) {
                taskOut.b = uvec2(0x61, 0x62);
            } else if (index == 12) {
                taskOut.c = uvec3(0x63, 0x64, 0x65);
            } else if (index == 2) {
                taskOut.d = uvec4(0x66, 0x67, 0x68, 0x69);

                taskOut.not_used = uvec3(0xEE, 0xEE, 0xEE);
                taskOut.also_not_used = 0xEE;
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint a;
            uvec2 b;
            uvec3 not_used;
            uvec3 c;
            uvec4 d;
            uint also_not_used;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[10];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 31) {
                result[0] = taskIn.a;
            } else if (index == 20) {
                result[1] = taskIn.b.x;
                result[2] = taskIn.b.y;
            } else if (index == 8) {
                result[3] = taskIn.c.x;
                result[4] = taskIn.c.y;
                result[5] = taskIn.c.z;
            } else if (index == 7) {
                result[6] = taskIn.d.x;
                result[7] = taskIn.d.y;
                result[8] = taskIn.d.z;
                result[9] = taskIn.d.w;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.some_not_used",
    .start = task_memory_some_not_used,
    .no_image = true,
};

static void
task_memory_load_output_struct(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        struct S {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        };

        taskNV out Task {
            S s;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 28) {
                taskOut.s.a = 0x70;
            } else if (index == 21) {
                taskOut.s.b = uvec2(0x71, 0x72);
            } else if (index == 12) {
                taskOut.s.c = uvec3(0x73, 0x74, 0x75);
            } else if (index == 2) {
                taskOut.s.d = uvec4(0x76, 0x77, 0x78, 0x79);
            }

            barrier();

            if (index == 2) {
                taskOut.s.a -= 0x10;
            } else if (index == 12) {
                taskOut.s.b -= 0x10;
            } else if (index == 21) {
                taskOut.s.c -= 0x10;
            } else if (index == 28) {
                taskOut.s.d -= 0x10;
            }

            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        struct S {
            uint a;
            uvec2 b;
            uvec3 c;
            uvec4 d;
        };

        taskNV in Task {
            S s;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[10];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 31) {
                result[0] = taskIn.s.a;
            } else if (index == 20) {
                result[1] = taskIn.s.b.x;
                result[2] = taskIn.s.b.y;
            } else if (index == 8) {
                result[3] = taskIn.s.c.x;
                result[4] = taskIn.s.c.y;
                result[5] = taskIn.s.c.z;
            } else if (index == 7) {
                result[6] = taskIn.s.d.x;
                result[7] = taskIn.s.d.y;
                result[8] = taskIn.s.d.z;
                result[9] = taskIn.s.d.w;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.load_output_struct",
    .start = task_memory_load_output_struct,
    .no_image = true,
};


static void
task_memory_many_meshes(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint value;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            taskOut.value = 0x60;
            gl_TaskCountNV = 4;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint value;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[4];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            uint mesh_id = gl_WorkGroupID.x;
            if (index == 0) {
                result[mesh_id] = taskIn.value + mesh_id;
            }
            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.many_meshes",
    .start = task_memory_many_meshes,
    .no_image = true,
};


static void
task_memory_many_tasks(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint task_id;
            uint value;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            taskOut.task_id = gl_WorkGroupID.x;
            taskOut.value = gl_WorkGroupID.x * 0x10 + 0x60;
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint task_id;
            uint value;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[4];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 0) {
                result[taskIn.task_id] = taskIn.value;
            }
            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x70, 0x80, 0x90,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .group_count_x = 4,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.many_tasks",
    .start = task_memory_many_tasks,
    .no_image = true,
};


static void
task_memory_many_tasks_and_meshes(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uint task_id;
            uint value;
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            taskOut.task_id = gl_WorkGroupID.x;
            taskOut.value = gl_WorkGroupID.x * 0x10 + 0x60;
            gl_TaskCountNV = 4;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint task_id;
            uint value;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[16];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            uint mesh_id = gl_WorkGroupID.x;
            if (index == 0) {
                result[taskIn.task_id * 4 + mesh_id] = taskIn.value + mesh_id;
            }
            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC,
    };

    uint32_t expected[] = {
        0x60, 0x61, 0x62, 0x63,
        0x70, 0x71, 0x72, 0x73,
        0x80, 0x81, 0x82, 0x83,
        0x90, 0x91, 0x92, 0x93,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .group_count_x = 4,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.many_tasks_and_meshes",
    .start = task_memory_many_tasks_and_meshes,
    .no_image = true,
};


static void
task_memory_large_array(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        taskNV out Task {
            uvec4 v[1024];  // At least 16Kb, which is the minimum limit.
        } taskOut;

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 1) {
                for (uint i = 0; i < taskOut.v.length(); i++) {
                    taskOut.v[i] = uvec4(i, taskOut.v.length() - i - 1,
                                         i, taskOut.v.length() - i - 1);
                }
            }
            gl_TaskCountNV = 1;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uvec4 v[1024];
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[4];
        };

        void main()
        {
            uint index = gl_LocalInvocationID.x;
            if (index == 1) {
                uint sum[4] = { 0, 0, 0, 0 };
                for (int i = 0; i < taskIn.v.length(); i++) {
                    sum[0] += taskIn.v[i].x;
                    sum[1] += taskIn.v[i].y;
                    sum[2] += taskIn.v[i].z;
                    sum[3] += taskIn.v[i].w;
                }
                for (int i = 0; i < result.length(); i++)
                    result[i] = sum[i];
            }
            gl_PrimitiveCountNV = 0;
        }
    );

    // Sum of the first 1023 integers.
    uint32_t sum        = (1023 * 1024) / 2;

    uint32_t result[]   = { 0xCC, 0xCC, 0xCC, 0xCC };
    uint32_t expected[] = { sum,  sum,  sum,  sum  };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.large_array",
    .start = task_memory_large_array,
    .no_image = true,
};

/* https://gitlab.freedesktop.org/mesa/mesa/-/issues/7141 */
static void
task_memory_issue7141(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;

        taskNV out Task {
            uint a;
        } taskOut;

        void main()
        {
            taskOut.a = 0x60;
            gl_TaskCountNV = 9;
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 3) out;
        layout(max_primitives = 1) out;
        layout(triangles) out;

        taskNV in Task {
            uint a;
        } taskIn;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[1];
        };

        void main()
        {
            if (gl_WorkGroupID.x == 8)
                result[0] = taskIn.a; /* this reads uninitialized data */

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC,
    };

    uint32_t expected[] = {
        0x60,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &result,
        .storage_size = sizeof(result),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < ARRAY_LENGTH(expected); i++) {
        t_assertf(result[i] == expected[i],
                  "buffer mismatch at uint %u: found 0x%02x, "
                  "expected 0x%02x", i, result[i], expected[i]);
    }
}

test_define {
    .name = "func.mesh.nv.task_memory.issue7141",
    .start = task_memory_issue7141,
    .no_image = true,
};
