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

#include "src/tests/func/mesh/ext/workgroup-id-spirv.h"

static void
workgroup_id_mesh(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 3) out;
        layout(max_primitives = 1) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
            int triangles = gl_WorkGroupID.x < 2 ? 1 : 0;
            SetMeshOutputsEXT(triangles * 3, triangles);

            switch (gl_WorkGroupID.x) {
            case 0: {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);

                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                color[0] = vec4(1, 0, 0, 1);
                color[1] = vec4(0, 1, 0, 1);
                color[2] = vec4(0, 0, 1, 1);

                break;
            }

            case 1: {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                color[0] = vec4(0, 1, 1, 1);
                color[1] = vec4(1, 0, 1, 1);
                color[2] = vec4(1, 1, 0, 1);

                break;
            }

            default:
                break;
            }
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .group_count_x = 3,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.workgroup_id.mesh",
    .start = workgroup_id_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};


static void
workgroup_id_task(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;

        taskPayloadSharedEXT struct Task {
            uvec3 parentID;
        } taskOut;

        void main()
        {
            taskOut.parentID = gl_WorkGroupID;
            EmitMeshTasksEXT(gl_WorkGroupID.x < 3 ? 1 : 0, 1, 1);
        }
    );
    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 3) out;
        layout(max_primitives = 1) out;
        layout(triangles) out;

        taskPayloadSharedEXT struct Task {
            uvec3 parentID;
        } taskIn;

        layout(location = 0) out vec4 color[];

        void main()
        {
            vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);

            int triangles = taskIn.parentID.x < 2 ? 1 : 0;
            SetMeshOutputsEXT(triangles * 3, triangles);

            switch (taskIn.parentID.x) {
            case 0: {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);

                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                color[0] = vec4(1, 0, 0, 1);
                color[1] = vec4(0, 1, 0, 1);
                color[2] = vec4(0, 0, 1, 1);

                break;
            }

            case 1: {
                gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                color[0] = vec4(0, 1, 1, 1);
                color[1] = vec4(1, 0, 1, 1);
                color[2] = vec4(1, 1, 0, 1);

                break;
            }

            default:
                break;
            }
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .group_count_x = 4,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.workgroup_id.task",
    .start = workgroup_id_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};
