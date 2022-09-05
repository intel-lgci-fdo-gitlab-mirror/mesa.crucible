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

#include "src/tests/func/mesh/nv/clipculldistance-spirv.h"

static VkShaderModule
get_clipdistance_1_shader(void)
{
    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            gl_PrimitiveCountNV = 4;

            for (int i = 0; i < 12; ++i)
                gl_PrimitiveIndicesNV[i] = i;

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesNV[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            gl_MeshVerticesNV[0].gl_ClipDistance[0] = 1;
            gl_MeshVerticesNV[1].gl_ClipDistance[0] = 1;
            gl_MeshVerticesNV[2].gl_ClipDistance[0] = 1;

            gl_MeshVerticesNV[3].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[4].gl_ClipDistance[0] =  1;
            gl_MeshVerticesNV[5].gl_ClipDistance[0] =  1;

            gl_MeshVerticesNV[6].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[7].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[8].gl_ClipDistance[0] =  1;

            gl_MeshVerticesNV[9].gl_ClipDistance[0]  = -1;
            gl_MeshVerticesNV[10].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[11].gl_ClipDistance[0] = -1;

            color[0] = vec4(1, 1, 1, 1);
            color[1] = vec4(1, 1, 1, 1);
            color[2] = vec4(1, 1, 1, 1);

            color[3] = vec4(1, 0, 0, 1);
            color[4] = vec4(1, 0, 0, 1);
            color[5] = vec4(1, 0, 0, 1);

            color[6] = vec4(0, 1, 0, 1);
            color[7] = vec4(0, 1, 0, 1);
            color[8] = vec4(0, 1, 0, 1);

            color[9]  = vec4(0, 0, 1, 1);
            color[10] = vec4(0, 0, 1, 1);
            color[11] = vec4(0, 0, 1, 1);
        }
    );

    return mesh;
}

static void
clipdistance_1(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = get_clipdistance_1_shader();

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.nv.clipdistance.1",
    .start = clipdistance_1,
    .image_filename = "func.mesh.clipdistance.ref.png",
};

static void
clipdistance_5(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        out gl_MeshPerVertexNV {
            vec4  gl_Position;
            float gl_ClipDistance[5];
        } gl_MeshVerticesNV[];

        layout(location = 0) out vec4 color[];

        void main()
        {
            gl_PrimitiveCountNV = 4;

            for (int i = 0; i < 12; ++i)
                gl_PrimitiveIndicesNV[i] = i;

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesNV[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            for (int i = 0; i < 5; ++i) {
                gl_MeshVerticesNV[0].gl_ClipDistance[i] = 1 + i;
                gl_MeshVerticesNV[1].gl_ClipDistance[i] = 1 + i;
                gl_MeshVerticesNV[2].gl_ClipDistance[i] = 1 + i;

                gl_MeshVerticesNV[3].gl_ClipDistance[i] = -1 - i;
                gl_MeshVerticesNV[4].gl_ClipDistance[i] =  1 + i;
                gl_MeshVerticesNV[5].gl_ClipDistance[i] =  1 + i;

                gl_MeshVerticesNV[6].gl_ClipDistance[i] = -1 - i;
                gl_MeshVerticesNV[7].gl_ClipDistance[i] = -1 - i;
                gl_MeshVerticesNV[8].gl_ClipDistance[i] =  1 + i;

                gl_MeshVerticesNV[9].gl_ClipDistance[i]  = -1 - i;
                gl_MeshVerticesNV[10].gl_ClipDistance[i] = -1 - i;
                gl_MeshVerticesNV[11].gl_ClipDistance[i] = -1 - i;
            }

            color[0] = vec4(1, 1, 1, 1);
            color[1] = vec4(1, 1, 1, 1);
            color[2] = vec4(1, 1, 1, 1);

            color[3] = vec4(1, 0, 0, 1);
            color[4] = vec4(1, 0, 0, 1);
            color[5] = vec4(1, 0, 0, 1);

            color[6] = vec4(0, 1, 0, 1);
            color[7] = vec4(0, 1, 0, 1);
            color[8] = vec4(0, 1, 0, 1);

            color[9]  = vec4(0, 0, 1, 1);
            color[10] = vec4(0, 0, 1, 1);
            color[11] = vec4(0, 0, 1, 1);
        }
    );

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.nv.clipdistance.5",
    .start = clipdistance_5,
    .image_filename = "func.mesh.clipdistance.ref.png",
};

static void
culldistance_1(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            gl_PrimitiveCountNV = 4;

            for (int i = 0; i < 12; ++i)
                gl_PrimitiveIndicesNV[i] = i;

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesNV[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            gl_MeshVerticesNV[0].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[1].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[2].gl_CullDistance[0] = 1;

            gl_MeshVerticesNV[3].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[4].gl_CullDistance[0] =  1;
            gl_MeshVerticesNV[5].gl_CullDistance[0] =  1;

            gl_MeshVerticesNV[6].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[7].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[8].gl_CullDistance[0] =  1;

            gl_MeshVerticesNV[9].gl_CullDistance[0]  = -1;
            gl_MeshVerticesNV[10].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[11].gl_CullDistance[0] = -1;

            color[0] = vec4(1, 1, 1, 1);
            color[1] = vec4(1, 1, 1, 1);
            color[2] = vec4(1, 1, 1, 1);

            color[3] = vec4(1, 0, 0, 1);
            color[4] = vec4(1, 0, 0, 1);
            color[5] = vec4(1, 0, 0, 1);

            color[6] = vec4(0, 1, 0, 1);
            color[7] = vec4(0, 1, 0, 1);
            color[8] = vec4(0, 1, 0, 1);

            color[9]  = vec4(0, 0, 1, 1);
            color[10] = vec4(0, 0, 1, 1);
            color[11] = vec4(0, 0, 1, 1);
        }
    );

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.nv.culldistance.1",
    .start = culldistance_1,
    .image_filename = "func.mesh.culldistance.ref.png",
};

static void
culldistance_5(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        out gl_MeshPerVertexNV {
            vec4  gl_Position;
            float gl_CullDistance[5];
        } gl_MeshVerticesNV[];

        void main()
        {
            gl_PrimitiveCountNV = 4;

            for (int i = 0; i < 12; ++i)
                gl_PrimitiveIndicesNV[i] = i;

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesNV[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            for (int i = 0; i < 5; ++i) {
                gl_MeshVerticesNV[0].gl_CullDistance[i] = 1 + i;
                gl_MeshVerticesNV[1].gl_CullDistance[i] = 1 + i;
                gl_MeshVerticesNV[2].gl_CullDistance[i] = 1 + i;

                gl_MeshVerticesNV[3].gl_CullDistance[i] = -1 - i;
                gl_MeshVerticesNV[4].gl_CullDistance[i] =  1 + i;
                gl_MeshVerticesNV[5].gl_CullDistance[i] =  1 + i;

                gl_MeshVerticesNV[6].gl_CullDistance[i] = -1 - i;
                gl_MeshVerticesNV[7].gl_CullDistance[i] = -1 - i;
                gl_MeshVerticesNV[8].gl_CullDistance[i] =  1 + i;

                gl_MeshVerticesNV[9].gl_CullDistance[i]  = -1 - i;
                gl_MeshVerticesNV[10].gl_CullDistance[i] = -1 - i;
                gl_MeshVerticesNV[11].gl_CullDistance[i] = -1 - i;
            }

            color[0] = vec4(1, 1, 1, 1);
            color[1] = vec4(1, 1, 1, 1);
            color[2] = vec4(1, 1, 1, 1);

            color[3] = vec4(1, 0, 0, 1);
            color[4] = vec4(1, 0, 0, 1);
            color[5] = vec4(1, 0, 0, 1);

            color[6] = vec4(0, 1, 0, 1);
            color[7] = vec4(0, 1, 0, 1);
            color[8] = vec4(0, 1, 0, 1);

            color[9]  = vec4(0, 0, 1, 1);
            color[10] = vec4(0, 0, 1, 1);
            color[11] = vec4(0, 0, 1, 1);
        }
    );

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.nv.culldistance.5",
    .start = culldistance_5,
    .image_filename = "func.mesh.culldistance.ref.png",
};

static void
clipdistance_1_culldistance_1(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            gl_PrimitiveCountNV = 4;

            for (int i = 0; i < 12; ++i)
                gl_PrimitiveIndicesNV[i] = i;

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesNV[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesNV[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            gl_MeshVerticesNV[0].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[1].gl_ClipDistance[0] =  1;
            gl_MeshVerticesNV[2].gl_ClipDistance[0] =  1;

            gl_MeshVerticesNV[3].gl_ClipDistance[0] = -1;
            gl_MeshVerticesNV[4].gl_ClipDistance[0] =  1;
            gl_MeshVerticesNV[5].gl_ClipDistance[0] = -1;

            gl_MeshVerticesNV[6].gl_ClipDistance[0] = 1;
            gl_MeshVerticesNV[7].gl_ClipDistance[0] = 1;
            gl_MeshVerticesNV[8].gl_ClipDistance[0] = 1;

            gl_MeshVerticesNV[9].gl_ClipDistance[0]  = 1;
            gl_MeshVerticesNV[10].gl_ClipDistance[0] = 1;
            gl_MeshVerticesNV[11].gl_ClipDistance[0] = 1;


            gl_MeshVerticesNV[0].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[1].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[2].gl_CullDistance[0] = 1;

            gl_MeshVerticesNV[3].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[4].gl_CullDistance[0] = 1;
            gl_MeshVerticesNV[5].gl_CullDistance[0] = 1;

            gl_MeshVerticesNV[6].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[7].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[8].gl_CullDistance[0] = -1;

            gl_MeshVerticesNV[9].gl_CullDistance[0]  = -1;
            gl_MeshVerticesNV[10].gl_CullDistance[0] = -1;
            gl_MeshVerticesNV[11].gl_CullDistance[0] =  1;

            color[0] = vec4(1, 1, 1, 1);
            color[1] = vec4(1, 1, 1, 1);
            color[2] = vec4(1, 1, 1, 1);

            color[3] = vec4(1, 0, 0, 1);
            color[4] = vec4(1, 0, 0, 1);
            color[5] = vec4(1, 0, 0, 1);

            color[6] = vec4(0, 1, 0, 1);
            color[7] = vec4(0, 1, 0, 1);
            color[8] = vec4(0, 1, 0, 1);

            color[9]  = vec4(0, 0, 1, 1);
            color[10] = vec4(0, 0, 1, 1);
            color[11] = vec4(0, 0, 1, 1);
        }
    );

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.nv.clipdistance_culldistance.1",
    .start = clipdistance_1_culldistance_1,
    .image_filename = "func.mesh.clipdistance_culldistance.ref.png",
};

static void
clipdistance_1_fs(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = get_clipdistance_1_shader();

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;
        layout(location = 0) in vec4 v_color;

        void main()
        {
            if (gl_ClipDistance[0] > 0.5)
                f_color = v_color;
            else
                f_color = vec4(0.6, 0.6, 0.6, 1);
        }
    );


    simple_mesh_pipeline_options_t opts = {
            .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.nv.clipdistance.1.fs",
    .start = clipdistance_1_fs,
    .image_filename = "func.mesh.clipdistance.fs.ref.png",
};
