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

#include "src/tests/func/mesh/buffers-spirv.h"

static void
buffers_mesh_ssbo_read(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(set = 0, binding = 0) buffer Storage {
            uint indices[6];
        };

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = 2;

                for (int i = 0; i < 6; i++)
                    gl_PrimitiveIndicesNV[i] = indices[i];

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);
            }
        }
    );

    uint32_t indices[] = {
        0, 1, 2,
        3, 4, 5,
    };

    simple_mesh_pipeline_options_t opts = {
        .storage = &indices,
        .storage_size = sizeof(indices),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.buffers.mesh_ssbo_read",
    .start = buffers_mesh_ssbo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_mesh_ssbo_write(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(set = 0, binding = 0) buffer Storage {
            uint indices[6];
        };

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = 2;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;
                gl_PrimitiveIndicesNV[3] = 3;
                gl_PrimitiveIndicesNV[4] = 4;
                gl_PrimitiveIndicesNV[5] = 5;

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);

                for (int i = 0; i < 6; i++)
                    indices[i] = 5 - i;
            }
        }
    );

    uint32_t indices[] = {
        0, 1, 2,
        3, 4, 5,
    };

    simple_mesh_pipeline_options_t opts = {
        .storage = &indices,
        .storage_size = sizeof(indices),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    for (unsigned i = 0; i < 6; ++i)
        assert(indices[i] == 5 - i);
}

test_define {
    .name = "func.mesh.buffers.mesh_ssbo_write",
    .start = buffers_mesh_ssbo_write,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_mesh_ubo_read(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        QO_EXTENSION GL_EXT_scalar_block_layout : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(std430, set = 0, binding = 0) uniform UBO {
            uint indices[6];
        };

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = 2;

                for (int i = 0; i < 6; i++)
                    gl_PrimitiveIndicesNV[i] = indices[i];

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);
            }
        }
    );

    uint32_t indices[] = {
        0, 1, 2,
        3, 4, 5,
    };

    simple_mesh_pipeline_options_t opts = {
        .uniform_data = &indices,
        .uniform_data_size = sizeof(indices),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.buffers.mesh_ubo_read",
    .start = buffers_mesh_ubo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ssbo_read(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 2) in;

        layout(set = 0, binding = 0) buffer Storage {
            uint tc;
        };

        taskNV out Task {
            uint primitives;
        } taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1) {
                gl_TaskCountNV = tc - 71;
                taskOut.primitives = 2;
            }
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint primitives;
        } taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = taskIn.primitives;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;
                gl_PrimitiveIndicesNV[3] = 3;
                gl_PrimitiveIndicesNV[4] = 4;
                gl_PrimitiveIndicesNV[5] = 5;

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);
            }
        }
    );

    uint32_t task_count[] = {
        72,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .storage = &task_count,
        .storage_size = sizeof(task_count),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.buffers.task_ssbo_read",
    .start = buffers_task_ssbo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ubo_read(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 2) in;

        layout(set = 0, binding = 0) uniform UBO {
            uint tc;
        };

        taskNV out Task {
            uint primitives;
        } taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1) {
                gl_TaskCountNV = tc - 71;
                taskOut.primitives = 2;
            }
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint primitives;
        } taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = taskIn.primitives;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;
                gl_PrimitiveIndicesNV[3] = 3;
                gl_PrimitiveIndicesNV[4] = 4;
                gl_PrimitiveIndicesNV[5] = 5;

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);
            }
        }
    );

    uint32_t task_count[] = {
        72,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .uniform_data = &task_count,
        .uniform_data_size = sizeof(task_count),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.buffers.task_ubo_read",
    .start = buffers_task_ubo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ubo_read_ssbo_write(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 2) in;

        layout(set = 1, binding = 1) uniform UBO {
            uint tc;
        };

        layout(set = 0, binding = 0) buffer SSBO {
            uint tc_ssbo;
        };

        taskNV out Task {
            uint primitives;
        } taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1) {
                gl_TaskCountNV = tc - 71;
                tc_ssbo = tc * 2;
                taskOut.primitives = 2;
            }
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        taskNV in Task {
            uint primitives;
        } taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveCountNV = taskIn.primitives;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;
                gl_PrimitiveIndicesNV[3] = 3;
                gl_PrimitiveIndicesNV[4] = 4;
                gl_PrimitiveIndicesNV[5] = 5;

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 1);
                per_vertex[1].color = vec4(0, 1, 0, 1);
                per_vertex[2].color = vec4(0, 0, 1, 1);
                per_vertex[3].color = vec4(0, 1, 1, 1);
                per_vertex[4].color = vec4(1, 0, 1, 1);
                per_vertex[5].color = vec4(1, 1, 0, 1);
            }
        }
    );

    uint32_t task_count[] = {
        72,
    };

    uint32_t task_count_mult[] = {
        13,
    };

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .uniform_data = &task_count,
        .uniform_data_size = sizeof(task_count),
        .storage = &task_count_mult,
        .storage_size = sizeof(task_count_mult),
    };

    run_simple_mesh_pipeline(mesh, &opts);

    assert(task_count_mult[0] == 72 * 2);
}

test_define {
    .name = "func.mesh.buffers.task_ubo_read_ssbo_write",
    .start = buffers_task_ubo_read_ssbo_write,
    .image_filename = "func.mesh.basic.ref.png",
};

