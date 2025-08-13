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

#include "src/tests/func/mesh/ext/viewportindex-spirv.h"

static VkShaderModule
get_mesh_shader(void)
{
    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            SetMeshOutputsEXT(12, 4);

            for (int i = 0; i < 4; ++i)
                gl_PrimitiveTriangleIndicesEXT[i] = uvec3(i * 3 + 0, i * 3 + 1, i * 3 + 2);

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesEXT[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesEXT[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesEXT[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            gl_MeshPrimitivesEXT[0].gl_ViewportIndex = 0;
            gl_MeshPrimitivesEXT[1].gl_ViewportIndex = 1;
            gl_MeshPrimitivesEXT[2].gl_ViewportIndex = 0;
            gl_MeshPrimitivesEXT[3].gl_ViewportIndex = 1;

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
    t_require_ext("VK_EXT_mesh_shader");

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
    .name = "func.mesh.ext.viewport_index",
    .start = viewport_index,
    .image_filename = "func.mesh.viewport_index.ref.png",
    .mesh_shader = true,
};

static void
viewport_index_fs(void)
{
    t_require_ext("VK_EXT_mesh_shader");

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

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;
        layout(location = 0) in vec4 v_color;

        void main()
        {
            if (gl_ViewportIndex == 1)
                f_color = v_color;
            else
                f_color = vec4(0.6, 0.6, 0.6, 1);
        }
    );

    simple_mesh_pipeline_options_t opts = {
            .viewport_state = &viewport_state,
            .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.viewport_index.fs",
    .start = viewport_index_fs,
    .image_filename = "func.mesh.viewport_index.fs.ref.png",
    .mesh_shader = true,
};

static void
viewport_index_primitive_id_fs(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            SetMeshOutputsEXT(12, 4);

            for (int i = 0; i < 4; ++i)
                gl_PrimitiveTriangleIndicesEXT[i] = uvec3(i * 3 + 0, i * 3 + 1, i * 3 + 2);

            for (int i = 0; i < 4; ++i) {
                gl_MeshVerticesEXT[i * 3 + 0].gl_Position = vec4(-0.5f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesEXT[i * 3 + 1].gl_Position = vec4(-1.0f,   0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
                gl_MeshVerticesEXT[i * 3 + 2].gl_Position = vec4(-0.75f, -0.25f, 0.0f, 1.0f) + i * vec4(0.5, 0, 0, 0);
            }

            gl_MeshPrimitivesEXT[0].gl_ViewportIndex = 0;
            gl_MeshPrimitivesEXT[1].gl_ViewportIndex = 1;
            gl_MeshPrimitivesEXT[2].gl_ViewportIndex = 0;
            gl_MeshPrimitivesEXT[3].gl_ViewportIndex = 1;

            gl_MeshPrimitivesEXT[0].gl_PrimitiveID = 7;
            gl_MeshPrimitivesEXT[1].gl_PrimitiveID = 3;
            gl_MeshPrimitivesEXT[2].gl_PrimitiveID = 9;
            gl_MeshPrimitivesEXT[3].gl_PrimitiveID = 2;

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

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;
        layout(location = 0) in vec4 v_color;

        void main()
        {
            if (gl_ViewportIndex == 1)
                f_color = v_color;
            else
                f_color = vec4(0.6, 0.6, 0.6, 1);
        }
    );

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
            .fs = fs,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

test_define {
    .name = "func.mesh.ext.viewport_index.primitive_id.fs",
    .start = viewport_index_primitive_id_fs,
    .image_filename = "func.mesh.viewport_index.fs.ref.png",
    .mesh_shader = true,
};

static void
run_viewport_mesh(VkShaderModule mesh, VkPipelineShaderStageCreateInfo *mesh_create_info)
{
#define OFF 30
    unsigned width  = t_width  / 2 - OFF;
    unsigned height = t_height / 2 - OFF;
    unsigned x = t_width  / 2 + OFF;
    unsigned y = t_height / 2 + OFF;
#undef OFF

    VkViewport vps[4] = {
            {
                    .x = 0,
                    .y = 0,
                    .width = width,
                    .height = height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
            {
                    .x = x,
                    .y = 0,
                    .width = width,
                    .height = height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
            {
                    .x = 0,
                    .y = y,
                    .width = width,
                    .height = height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
            {
                    .x = x,
                    .y = y,
                    .width = width,
                    .height = height,
                    .minDepth = 0.0,
                    .maxDepth = 1.0
            },
    };

    VkRect2D scissors[4] =  {
            {
                    { 0, 0 },
                    { width, height }
            },
            {
                    { x, 0 },
                    { width, height }
            },
            {
                    { 0, y },
                    { width, height }
            },
            {
                    { x, y },
                    { width, height }
            },
    };

    VkPipelineViewportStateCreateInfo viewport_state;
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 4;
    viewport_state.pViewports = vps;
    viewport_state.scissorCount = 4;
    viewport_state.pScissors = scissors;

    simple_mesh_pipeline_options_t opts = {
            .viewport_state = &viewport_state,
            .mesh_create_info = mesh_create_info,
    };

    run_simple_mesh_pipeline(mesh, &opts);
}

static void
viewport_index_wg_1(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 192) out;
        layout(max_primitives = 64) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];
#define PRIMS 64
#define DIM 8

        void gen_prim(in int prim)
        {
            int y = prim / DIM;
            int x = prim % DIM;

            gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(prim * 3 + 0, prim * 3 + 1, prim * 3 + 2);

            gl_MeshVerticesEXT[prim * 3 + 0].gl_Position = vec4(-0.75f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 1].gl_Position = vec4(-1.00f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 2].gl_Position = vec4(-0.875f, -1.00f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);

            gl_MeshPrimitivesEXT[prim].gl_ViewportIndex = (prim % 7) % 4;

            vec4 col;

            if (prim < 16)
                col = vec4(1, 0, 0, 1);
            else if (prim < 32)
                col = vec4(0, 1, 0, 1);
            else if (prim < 48)
                col = vec4(0, 0, 1, 1);
            else if (prim < 64)
                col = vec4(1, 1, 1, 1);

            color[prim * 3 + 0] = col;
            color[prim * 3 + 1] = col;
            color[prim * 3 + 2] = col;
        }

        void main()
        {
            SetMeshOutputsEXT(PRIMS * 3, PRIMS);

            for (int prim = 0; prim < PRIMS; ++prim)
                gen_prim(prim);
        }
    );

    run_viewport_mesh(mesh, NULL);
}

test_define {
    .name = "func.mesh.ext.viewport_index.wg.1",
    .start = viewport_index_wg_1,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .mesh_shader = true,
};

static void
viewport_index_wg_32(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 32) in;
        layout(max_vertices = 192) out;
        layout(max_primitives = 64) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];
#define PRIMS 64
#define DIM 8

        void gen_prim(in int prim)
        {
            int y = prim / DIM;
            int x = prim % DIM;

            gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(prim * 3 + 0, prim * 3 + 1, prim * 3 + 2);

            gl_MeshVerticesEXT[prim * 3 + 0].gl_Position = vec4(-0.75f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 1].gl_Position = vec4(-1.00f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 2].gl_Position = vec4(-0.875f, -1.00f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);

            gl_MeshPrimitivesEXT[prim].gl_ViewportIndex = (prim % 7) % 4;

            vec4 col;

            if (prim < 16)
                col = vec4(1, 0, 0, 1);
            else if (prim < 32)
                col = vec4(0, 1, 0, 1);
            else if (prim < 48)
                col = vec4(0, 0, 1, 1);
            else if (prim < 64)
                col = vec4(1, 1, 1, 1);

            color[prim * 3 + 0] = col;
            color[prim * 3 + 1] = col;
            color[prim * 3 + 2] = col;
        }

        void main()
        {
            int local_x = int(gl_LocalInvocationID.x);
            int size_x = int(gl_WorkGroupSize.x);

            SetMeshOutputsEXT(PRIMS * 3, PRIMS);

            gen_prim(local_x);
            gen_prim(local_x + size_x);
        }
    );

    run_viewport_mesh(mesh, NULL);
}

test_define {
    .name = "func.mesh.ext.viewport_index.wg.32",
    .start = viewport_index_wg_32,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .mesh_shader = true,
};

static void
viewport_index_wg_gen(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x_id = 17) in;
        layout(max_vertices = 192) out;
        layout(max_primitives = 64) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];
#define PRIMS 64
#define DIM 8

        void gen_prim(in int prim)
        {
            int y = prim / DIM;
            int x = prim % DIM;

            gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(prim * 3 + 0, prim * 3 + 1, prim * 3 + 2);

            gl_MeshVerticesEXT[prim * 3 + 0].gl_Position = vec4(-0.75f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 1].gl_Position = vec4(-1.00f,  -0.75f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);
            gl_MeshVerticesEXT[prim * 3 + 2].gl_Position = vec4(-0.875f, -1.00f, 0.0f, 1.0f) + vec4(x * 0.25, y * 0.25, 0, 0);

            gl_MeshPrimitivesEXT[prim].gl_ViewportIndex = (prim % 7) % 4;

            vec4 col;

            if (prim < 16)
                col = vec4(1, 0, 0, 1);
            else if (prim < 32)
                col = vec4(0, 1, 0, 1);
            else if (prim < 48)
                col = vec4(0, 0, 1, 1);
            else if (prim < 64)
                col = vec4(1, 1, 1, 1);

            color[prim * 3 + 0] = col;
            color[prim * 3 + 1] = col;
            color[prim * 3 + 2] = col;
        }

        void main()
        {
            int local_x = int(gl_LocalInvocationID.x);
            int size_x = int(gl_WorkGroupSize.x);

            SetMeshOutputsEXT(PRIMS * 3, PRIMS);

            while (local_x < PRIMS) {
                gen_prim(local_x);
                local_x += size_x;
            }
        }
    );

    uint32_t local_size_x = *(uint32_t *)t_user_data;

    VkSpecializationMapEntry entry;
    entry.constantID = 17;
    entry.offset = 0;
    entry.size = sizeof(uint32_t);

    VkSpecializationInfo spec_info;
    spec_info.mapEntryCount = 1;
    spec_info.pMapEntries = &entry;
    spec_info.dataSize = sizeof(local_size_x);
    spec_info.pData = &local_size_x;

    VkPipelineShaderStageCreateInfo mesh_create_info;

    mesh_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    mesh_create_info.pNext = NULL;
    mesh_create_info.flags = 0;
    mesh_create_info.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    mesh_create_info.module = mesh;
    mesh_create_info.pName = "main";
    mesh_create_info.pSpecializationInfo = &spec_info;

    run_viewport_mesh(VK_NULL_HANDLE, &mesh_create_info);
}

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.1",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 1 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.2",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 2 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.3",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 3 },
    .mesh_shader = true,
};


test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.7",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 7 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.8",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 8 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.11",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 11 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.15",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 15 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.16",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 16 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.17",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 17 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.27",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 27 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.31",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 31 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.32",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 32 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.33",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 33 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.63",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 63 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.64",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 64 },
    .mesh_shader = true,
};

test_define {
    .name = "func.mesh.ext.viewport_index.wg.gen.65",
    .start = viewport_index_wg_gen,
    .image_filename = "func.mesh.viewport_index.wg.ref.png",
    .user_data = &(uint32_t){ 65 },
    .mesh_shader = true,
};
