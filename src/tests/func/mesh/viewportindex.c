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

#include "src/tests/func/mesh/viewportindex-spirv.h"

static VkShaderModule
get_mesh_shader(void)
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

            gl_MeshPrimitivesNV[0].gl_ViewportIndex = 0;
            gl_MeshPrimitivesNV[1].gl_ViewportIndex = 1;
            gl_MeshPrimitivesNV[2].gl_ViewportIndex = 0;
            gl_MeshPrimitivesNV[3].gl_ViewportIndex = 1;

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
viewport_index(void)
{
    t_require_ext("VK_NV_mesh_shader");

    VkShaderModule mesh = get_mesh_shader();

    VkViewport vps[2] = {
            {
                    .x = 0,
                    .y = 0,
                    .width = t_width / 2,
                    .height = t_height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
            {
                    .x = t_width / 2,
                    .y = 0,
                    .width = t_width / 2,
                    .height = t_height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
    };

    VkRect2D scissors[2] =  {
            {
                    { 0, 0 },
                    { t_width / 2, t_height }
            },
            {
                    { t_width / 2, 0 },
                    { t_width / 2, t_height }
            },
    };

    VkPipelineViewportStateCreateInfo viewport_state;
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 2;
    viewport_state.pViewports = vps;
    viewport_state.scissorCount = 2;
    viewport_state.pScissors = scissors;

    simple_mesh_pipeline_options_t opts = {
            .viewport_state = &viewport_state,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.viewport_index",
    .start = viewport_index,
    .image_filename = "func.mesh.viewport_index.ref.png",
};
