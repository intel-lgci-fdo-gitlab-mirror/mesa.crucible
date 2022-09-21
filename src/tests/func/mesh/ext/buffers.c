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

#include "src/tests/func/mesh/ext/buffers-spirv.h"

static void
buffers_mesh_ssbo_read(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
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
            SetMeshOutputsEXT(6, 2);

            if (gl_LocalInvocationID.x == 0) {
                for (int i = 0; i < 2; i++) {
                    gl_PrimitiveTriangleIndicesEXT[i] =
                            uvec3(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2]);
                }

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.mesh_ssbo_read",
    .start = buffers_mesh_ssbo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_mesh_ssbo_write(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
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
            SetMeshOutputsEXT(6, 2);

            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
                gl_PrimitiveTriangleIndicesEXT[1] = uvec3(3, 4, 5);

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.mesh_ssbo_write",
    .start = buffers_mesh_ssbo_write,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_mesh_ubo_read(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_EXTENSION GL_EXT_scalar_block_layout : require
        QO_TARGET_ENV spirv1.4
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
            SetMeshOutputsEXT(6, 2);

            if (gl_LocalInvocationID.x == 0) {
                for (int i = 0; i < 2; i++) {
                    gl_PrimitiveTriangleIndicesEXT[i] =
                            uvec3(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2]);
                }

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.mesh_ubo_read",
    .start = buffers_mesh_ubo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ssbo_read(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 2) in;

        layout(set = 0, binding = 0) buffer Storage {
            uint tc;
        };

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1)
                taskOut.primitives = 2;

            EmitMeshTasksEXT(tc - 71, 1, 1);
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            SetMeshOutputsEXT(6, taskIn.primitives);

            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
                gl_PrimitiveTriangleIndicesEXT[1] = uvec3(3, 4, 5);

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.task_ssbo_read",
    .start = buffers_task_ssbo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ubo_read(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 2) in;

        layout(set = 0, binding = 0) uniform UBO {
            uint tc;
        };

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1)
               taskOut.primitives = 2;

            EmitMeshTasksEXT(tc - 71, 1, 1);
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            SetMeshOutputsEXT(6, taskIn.primitives);

            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
                gl_PrimitiveTriangleIndicesEXT[1] = uvec3(3, 4, 5);

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.task_ubo_read",
    .start = buffers_task_ubo_read,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
buffers_task_ubo_read_ssbo_write(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 2) in;

        layout(set = 1, binding = 1) uniform UBO {
            uint tc;
        };

        layout(set = 0, binding = 0) buffer SSBO {
            uint tc_ssbo;
        };

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 1) {
                tc_ssbo = tc * 2;
                taskOut.primitives = 2;
            }

            EmitMeshTasksEXT(tc - 71, 1, 1);
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 4) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskIn;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            SetMeshOutputsEXT(6, taskIn.primitives);

            if (gl_LocalInvocationID.x == 0) {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
                gl_PrimitiveTriangleIndicesEXT[1] = uvec3(3, 4, 5);

                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.buffers.task_ubo_read_ssbo_write",
    .start = buffers_task_ubo_read_ssbo_write,
    .image_filename = "func.mesh.basic.ref.png",
};

