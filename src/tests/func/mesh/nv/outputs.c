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

#include "src/tests/func/mesh/outputs-spirv.h"

static void
outputs_per_vertex(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            gl_PrimitiveCountNV = 2;

            if (local < 6) {
                gl_PrimitiveIndicesNV[local] = local;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                color[0] = vec4(1, 0, 0, 1);
                color[1] = vec4(0, 1, 0, 1);
                color[2] = vec4(0, 0, 1, 1);
                color[3] = vec4(0, 1, 1, 1);
                color[4] = vec4(1, 0, 1, 1);
                color[5] = vec4(1, 1, 0, 1);
            }
        }
    );

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.outputs.per_vertex",
    .start = outputs_per_vertex,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_vertex_block(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            gl_PrimitiveCountNV = 2;

            if (local < 6) {
                gl_PrimitiveIndicesNV[local] = local;
            }
            if (local == 31) {
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

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.outputs.per_vertex_block",
    .start = outputs_per_vertex_block,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_primitive(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        perprimitiveNV layout(location = 4) out float alpha[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            gl_PrimitiveCountNV = 2;

            if (local < 6) {
                gl_PrimitiveIndicesNV[local] = local;
            }

            if (local < 2) {
                alpha[local] = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 0);
                per_vertex[1].color = vec4(0, 1, 0, 0);
                per_vertex[2].color = vec4(0, 0, 1, 0);
                per_vertex[3].color = vec4(0, 1, 1, 0);
                per_vertex[4].color = vec4(1, 0, 1, 0);
                per_vertex[5].color = vec4(1, 1, 0, 0);
            }
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(location = 0) in vec4 in_color;
        perprimitiveNV layout(location = 4) in float in_alpha;
        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = in_color;
            out_color.a = in_alpha;
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.outputs.per_primitive",
    .start = outputs_per_primitive,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_primitive_block(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        perprimitiveNV layout(location = 4) out PerPrimitive {
            float alpha;
        } per_primitive[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            gl_PrimitiveCountNV = 2;

            if (local < 6) {
                gl_PrimitiveIndicesNV[local] = local;
            }

            if (local < 2) {
                per_primitive[local].alpha = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 0);
                per_vertex[1].color = vec4(0, 1, 0, 0);
                per_vertex[2].color = vec4(0, 0, 1, 0);
                per_vertex[3].color = vec4(0, 1, 1, 0);
                per_vertex[4].color = vec4(1, 0, 1, 0);
                per_vertex[5].color = vec4(1, 1, 0, 0);
            }
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(location = 0) in vec4 in_color;
        perprimitiveNV layout(location = 4) in float in_alpha;
        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = in_color;
            out_color.a = in_alpha;
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.outputs.per_primitive_block",
    .start = outputs_per_primitive_block,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_write_packed_indices(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        void main()
        {
            if (gl_LocalInvocationID.x == 16) {
                gl_PrimitiveCountNV = 2;

                writePackedPrimitiveIndices4x8NV(0, 0x03020100);
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

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.outputs.write_packed_indices",
    .start = outputs_write_packed_indices,
    .image_filename = "func.mesh.basic.ref.png",
};

static void
outputs_per_primitive_unused(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out PerVertex {
            vec4 color;
        } per_vertex[];

        perprimitiveNV layout(location = 5) out float alphaprim[];
        layout(location = 4) out flat float alpha[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            gl_PrimitiveCountNV = 2;

            if (local < 6) {
                gl_PrimitiveIndicesNV[local] = local;
            }

            if (local < 2) {
                alphaprim[local] = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesNV[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesNV[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesNV[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesNV[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].color = vec4(1, 0, 0, 0);
                per_vertex[1].color = vec4(0, 1, 0, 0);
                per_vertex[2].color = vec4(0, 0, 1, 0);
                per_vertex[3].color = vec4(0, 1, 1, 0);
                per_vertex[4].color = vec4(1, 0, 1, 0);
                per_vertex[5].color = vec4(1, 1, 0, 0);
            }

            groupMemoryBarrier();
            barrier();

            alpha[0] = alphaprim[0];
            alpha[3] = alphaprim[1];
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_NV_mesh_shader : require
        layout(location = 0) in vec4 in_color;
        layout(location = 4) in flat float in_alpha;
        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = in_color;
            out_color.a = in_alpha;
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.outputs.per_primitive.unused",
    .start = outputs_per_primitive_unused,
    .image_filename = "func.mesh.basic.ref.png",
};
