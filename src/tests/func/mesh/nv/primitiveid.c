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

#include "src/tests/func/mesh/nv/primitiveid-spirv.h"

static void
primitive_id_fs(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

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

            gl_MeshPrimitivesNV[0].gl_PrimitiveID = 7;
            gl_MeshPrimitivesNV[1].gl_PrimitiveID = 3;
            gl_MeshPrimitivesNV[2].gl_PrimitiveID = 9;
            gl_MeshPrimitivesNV[3].gl_PrimitiveID = 2;
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;

        void main()
        {
            switch (gl_PrimitiveID) {
            case 7:
                f_color = vec4(1, 0, 0, 1);
                break;
            case 3:
                f_color = vec4(0, 1, 0, 1);
                break;
            case 9:
                f_color = vec4(0, 0, 1, 1);
                break;
            case 2:
                f_color = vec4(1, 1, 1, 1);
                break;
            default:
                f_color = vec4(0, 0, 0, 1);
                break;
            }
        }
    );

    simple_mesh_pipeline_options_t opts = {
            .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.nv.primitive_id.fs",
    .start = primitive_id_fs,
    .image_filename = "func.mesh.primitive_id.fs.ref.png",
};
