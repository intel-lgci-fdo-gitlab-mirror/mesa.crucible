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

#include "src/tests/func/mesh/workgroup-memory-spirv.h"

static void
workgroup_memory_mesh_uint(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        shared uint a;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[1];
        };

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                a = 0x11;
            }

            barrier();

            if (gl_LocalInvocationID.x == 31) {
                result[0] = a;
            }

            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC,
    };

    uint32_t expected[] = {
        0x11,
    };

    simple_mesh_pipeline_options_t opts = {
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
    .name = "func.mesh.workgroup_memory.mesh_uint",
    .start = workgroup_memory_mesh_uint,
    .no_image = true,
};

static void
workgroup_memory_task_uint(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;

        shared uint a;

        layout(set = 0, binding = 0) buffer Storage {
            uint result[1];
        };

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                a = 0x11;
            }

            barrier();

            if (gl_LocalInvocationID.x == 31) {
                result[0] = a;
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

        void main()
        {
            gl_PrimitiveCountNV = 0;
        }
    );

    uint32_t result[] = {
        0xCC,
    };

    uint32_t expected[] = {
        0x11,
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
    .name = "func.mesh.workgroup_memory.task_uint",
    .start = workgroup_memory_task_uint,
    .no_image = true,
};

