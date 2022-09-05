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

#include "src/tests/func/mesh/workgroup-id-spirv.h"

static void
workgroup_id_mesh(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 3) out;
        layout(max_primitives = 1) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);

            switch (gl_WorkGroupID.x) {
            case 0: {
                gl_PrimitiveCountNV = 1;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;

                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                color[0] = vec4(1, 0, 0, 1);
                color[1] = vec4(0, 1, 0, 1);
                color[2] = vec4(0, 0, 1, 1);

                break;
            }

            case 1: {
                gl_PrimitiveCountNV = 1;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                color[0] = vec4(0, 1, 1, 1);
                color[1] = vec4(1, 0, 1, 1);
                color[2] = vec4(1, 1, 0, 1);

                break;
            }

            default:
                gl_PrimitiveCountNV = 0;
                break;
            }
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .task_count = 3,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.workgroup_id.mesh",
    .start = workgroup_id_mesh,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
workgroup_id_task(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;

        taskNV out Task {
            uvec3 parentID;
        } taskOut;

        void main()
        {
            gl_TaskCountNV = gl_WorkGroupID.x < 3 ? 1 : 0;
            taskOut.parentID = gl_WorkGroupID;
        }
    );
    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 3) out;
        layout(max_primitives = 1) out;
        layout(triangles) out;

        taskNV in Task {
            uvec3 parentID;
        } taskIn;

        layout(location = 0) out vec4 color[];

        void main()
        {
            vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);

            switch (taskIn.parentID.x) {
            case 0: {
                gl_PrimitiveCountNV = 1;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;

                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                color[0] = vec4(1, 0, 0, 1);
                color[1] = vec4(0, 1, 0, 1);
                color[2] = vec4(0, 0, 1, 1);

                break;
            }

            case 1: {
                gl_PrimitiveCountNV = 1;

                gl_PrimitiveIndicesNV[0] = 0;
                gl_PrimitiveIndicesNV[1] = 1;
                gl_PrimitiveIndicesNV[2] = 2;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                color[0] = vec4(0, 1, 1, 1);
                color[1] = vec4(1, 0, 1, 1);
                color[2] = vec4(1, 1, 0, 1);

                break;
            }

            default:
                gl_PrimitiveCountNV = 0;
                break;
            }
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .task_count = 4,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.workgroup_id.task",
    .start = workgroup_id_task,
    .image_filename = "func.mesh.basic.ref.png",
};
