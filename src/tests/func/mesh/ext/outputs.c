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

#include "src/tests/func/mesh/ext/outputs-spirv.h"

static void
outputs_per_vertex_basic(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 32) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
    .name = "func.mesh.ext.outputs.per_vertex.basic",
    .start = outputs_per_vertex_basic,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_vertex_block(void)
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
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local == 31) {
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

    run_simple_mesh_pipeline(mesh, NULL);
}

test_define {
    .name = "func.mesh.ext.outputs.per_vertex.block",
    .start = outputs_per_vertex_block,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_primitive_basic(void)
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

        perprimitiveEXT layout(location = 4) out float alpha[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local < 2) {
                alpha[local] = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(location = 0) in vec4 in_color;
        perprimitiveEXT layout(location = 4) in float in_alpha;
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
    .name = "func.mesh.ext.outputs.per_primitive.basic",
    .start = outputs_per_primitive_basic,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_primitive_block(void)
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

        perprimitiveEXT layout(location = 4) out PerPrimitive {
            float alpha;
        } per_primitive[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local < 2) {
                per_primitive[local].alpha = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(location = 0) in vec4 in_color;
        perprimitiveEXT layout(location = 4) in float in_alpha;
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
    .name = "func.mesh.ext.outputs.per_primitive.block",
    .start = outputs_per_primitive_block,
    .image_filename = "func.mesh.basic.ref.png",
};


static void
outputs_per_primitive_unused(void)
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

        perprimitiveEXT layout(location = 5) out float alphaprim[];
        layout(location = 4) out flat float alpha[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local < 2) {
                alphaprim[local] = 1.0;
            }

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

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
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
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
    .name = "func.mesh.ext.outputs.per_primitive.unused",
    .start = outputs_per_primitive_unused,
    .image_filename = "func.mesh.basic.ref.png",
};

static void
outputs_per_vertex_block_compact_layout(void)
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
            float scale1;
            float scale2;
            vec3 color;
        } per_vertex[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_vertex[0].scale1 = 0.5;
                per_vertex[0].scale2 = 0.5;

                per_vertex[1].scale1 = 0.4;
                per_vertex[1].scale2 = 0.6;

                per_vertex[2].scale1 = 0.3;
                per_vertex[2].scale2 = 0.7;

                per_vertex[3].scale1 = 0.2;
                per_vertex[3].scale2 = 0.8;

                per_vertex[4].scale1 = 0.1;
                per_vertex[4].scale2 = 0.9;

                per_vertex[5].scale1 = 0.5;
                per_vertex[5].scale2 = 0.5;

                per_vertex[0].color = vec3(1, 0, 0);
                per_vertex[1].color = vec3(0, 1, 0);
                per_vertex[2].color = vec3(0, 0, 1);
                per_vertex[3].color = vec3(0, 1, 1);
                per_vertex[4].color = vec3(1, 0, 1);
                per_vertex[5].color = vec3(1, 1, 0);
            }
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4

        layout(location = 0) in per_vertex_interp {
            float scale1;
            float scale2;
            vec3 color;
        } in_data;

        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = vec4(in_data.color, 1.0) * (in_data.scale1 + in_data.scale2);
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.outputs.per_vertex.block_compact_layout",
    .start = outputs_per_vertex_block_compact_layout,
    .image_filename = "func.mesh.basic.ref.png",
};

static void
outputs_per_vertex_block_compact_layout_flat(void)
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
            flat float scale1;
            float scale2;
            vec3 color;
        } per_vertex[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                /* scale1 is flat, so an uninterpolated value from the first
                 * vertex will be propagated to the fragment shader, where it
                 * will be summed with interpolated scale2, giving us a value
                 * of 1.0
                 */
                per_vertex[0].scale1 = 0.5;
                per_vertex[0].scale2 = 0.5;

                per_vertex[1].scale1 = 0.4;
                per_vertex[1].scale2 = 0.5;

                per_vertex[2].scale1 = 0.3;
                per_vertex[2].scale2 = 0.5;

                per_vertex[3].scale1 = 0.2;
                per_vertex[3].scale2 = 0.8;

                per_vertex[4].scale1 = 0.1;
                per_vertex[4].scale2 = 0.8;

                per_vertex[5].scale1 = 0.5;
                per_vertex[5].scale2 = 0.8;

                per_vertex[0].color = vec3(1, 0, 0);
                per_vertex[1].color = vec3(0, 1, 0);
                per_vertex[2].color = vec3(0, 0, 1);
                per_vertex[3].color = vec3(0, 1, 1);
                per_vertex[4].color = vec3(1, 0, 1);
                per_vertex[5].color = vec3(1, 1, 0);
            }
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4

        layout(location = 0) in per_vertex_interp {
            flat float scale1;
            float scale2;
            vec3 color;
        } in_data;

        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = vec4(in_data.color, 1.0) * (in_data.scale1 + in_data.scale2);
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.outputs.per_vertex.block_compact_layout_flat",
    .start = outputs_per_vertex_block_compact_layout_flat,
    .image_filename = "func.mesh.basic.ref.png",
};

static void
outputs_per_primitive_block_compact_layout(void)
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

        perprimitiveEXT layout(location = 1) out PerPrimitive {
            float scale1;
            float scale2;
            vec4 offset;
        } per_primitive[];

        void main()
        {
            uint local = gl_LocalInvocationID.x;
            SetMeshOutputsEXT(6, 2);

            if (local < 2)
                gl_PrimitiveTriangleIndicesEXT[local] = uvec3(local * 3 + 0, local * 3 + 1, local * 3 + 2);

            if (local == 31) {
                vec4 scale = vec4(0.5, 0.5, 0.5, 1.0);
                vec4 pos_a = vec4(-0.5f, -0.5f, 0, 0);
                gl_MeshVerticesEXT[0].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[1].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_a;
                gl_MeshVerticesEXT[2].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_a;

                vec4 pos_b = vec4(0.5f, 0.5f, 0, 0);
                gl_MeshVerticesEXT[3].gl_Position = scale * vec4(0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[4].gl_Position = scale * vec4(-0.5f, 0.5f, 0.0f, 1.0f) + pos_b;
                gl_MeshVerticesEXT[5].gl_Position = scale * vec4(0.0f, -0.5f, 0.0f, 1.0f) + pos_b;

                per_primitive[0].scale1 = 0.5;
                per_primitive[0].scale2 = 0.5;
                per_primitive[0].offset = vec4(0, 0, 0, 0.2);

                per_primitive[1].scale1 = 0.4;
                per_primitive[1].scale2 = 0.6;
                per_primitive[1].offset = vec4(0, 0, 0, 0.2);

                per_vertex[0].color = vec4(1, 0, 0, 0.8);
                per_vertex[1].color = vec4(0, 1, 0, 0.8);
                per_vertex[2].color = vec4(0, 0, 1, 0.8);
                per_vertex[3].color = vec4(0, 1, 1, 0.8);
                per_vertex[4].color = vec4(1, 0, 1, 0.8);
                per_vertex[5].color = vec4(1, 1, 0, 0.8);
            }
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4

        layout(location = 0) in vec4 v_color;

        perprimitiveEXT layout(location = 1) in per_prim {
            float scale1;
            float scale2;
            vec4 offset;
        } in_per_prim;

        layout(location = 0) out vec4 out_color;

        void main()
        {
            out_color = v_color * (in_per_prim.scale1 + in_per_prim.scale2) + in_per_prim.offset;
        }
    );

    simple_mesh_pipeline_options_t opts = {
        .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.outputs.per_primitive.block_compact_layout",
    .start = outputs_per_primitive_block_compact_layout,
    .image_filename = "func.mesh.basic.ref.png",
};
