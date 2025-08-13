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

#include "src/tests/func/mesh/ext/basic-spirv.h"

static unsigned
parse_require_subgroup_size(const char *s)
{
    const char *p = strstr(s, "_require");
    if (p)
        return strtol(p + sizeof("_require") - 1, NULL, 10);
    else
        return 0;
}

static void
basic_mesh(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            SetMeshOutputsEXT(6, 2);

            if (gl_LocalInvocationID.x == 31) {
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

    simple_mesh_pipeline_options_t opts = {
        .required_subgroup_size = parse_require_subgroup_size(t_name),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.basic.mesh",
    .start = basic_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.mesh_require8",
    .start = basic_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.mesh_require16",
    .start = basic_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.mesh_require32",
    .start = basic_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.mesh_require64",
    .start = basic_mesh,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};



static void
basic_task(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule task = qoCreateShaderModuleGLSL(t_device, TASK,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 32) in;

        struct Task {
            uint primitives;
        };

        taskPayloadSharedEXT Task taskOut;

        void main()
        {
            if (gl_LocalInvocationID.x == 15)
                taskOut.primitives = 2;

            EmitMeshTasksEXT(1, 1, 1);
        }
    );

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 32) in;
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

            if (gl_LocalInvocationID.x == 31) {
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

    simple_mesh_pipeline_options_t opts = {
        .task = task,
        .required_subgroup_size = parse_require_subgroup_size(t_name),
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.basic.task",
    .start = basic_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.task_require8",
    .start = basic_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.task_require16",
    .start = basic_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.task_require32",
    .start = basic_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.basic.task_require64",
    .start = basic_task,
    .image_filename = "func.mesh.basic.ref.png",
    .mesh_shader = true,
};
