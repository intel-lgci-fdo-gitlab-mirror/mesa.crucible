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

#include <ctype.h>
#include "tapi/t.h"
#include "util/string.h"

#include "src/tests/func/mesh/multiview-spirv.h"

#define GET_DEVICE_FUNCTION_PTR(name) \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(t_device, #name); \
    t_assert(name != NULL);

typedef struct multiview_mesh_pipeline_options multiview_mesh_pipeline_options_t;

struct multiview_mesh_pipeline_options {
    VkShaderModule task;
};

static test_result_t
run_multiview_mesh_pipeline(VkShaderModule mesh,
                            const struct multiview_mesh_pipeline_options *_opts)
{
    t_require_ext("VK_NV_mesh_shader");
    t_require_ext("VK_NVX_multiview_per_view_attributes");

    /* Parse test name. Its format is:
     * name_which_can_contain_dots.max_views_in_decimal.view_mask_in_binary.qN
     */
    uint32_t view_mask = 0, bit = 0;
    char *test_name = strdup(t_name);
    char *pos = strrchr(test_name, '.');
    *pos = 0; /* drop .qN */
    pos--;
    while (*pos >= '0' && *pos <= '1') {
        if (*pos == '1')
            view_mask |= 1u << bit;
        bit++;
        pos--;
    }
    assert(*pos == '.');
    char *max_views_end = pos;
    pos--;
    while (isdigit(*pos))
        pos--;
    assert(*pos == '.');

    *max_views_end = 0;
    uint32_t max_views = atoi(pos + 1);
    *max_views_end = '.';

    const unsigned width = 128;
    const unsigned height = 128;

    VkPhysicalDeviceMeshShaderFeaturesNV features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV,
    };
    VkPhysicalDeviceFeatures2 pfeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features,
    };
    vkGetPhysicalDeviceFeatures2(t_physical_dev, &pfeatures);

    struct multiview_mesh_pipeline_options opts = {};
    if (_opts)
        opts = *_opts;
    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) in vec4 in_color;
        layout(location = 0) out vec4 out_color;
        void main()
        {
            out_color = in_color;
        }
    );

    if (!features.meshShader)
        t_skipf("meshShader not supported");
    if (opts.task != VK_NULL_HANDLE && !features.taskShader)
        t_skipf("taskShader not supported");

    GET_DEVICE_FUNCTION_PTR(vkCmdDrawMeshTasksNV);

    VkRenderPassMultiviewCreateInfo renderPassMultiviewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO_KHR,
        .pNext = NULL,
        .subpassCount = 1,
        .pViewMasks = &view_mask,
// TODO: test this
//        .dependencyCount = 0,
//        .pViewOffsets = NULL,
//        .correlationMaskCount = 0,
//        .pCorrelationMasks = NULL,
    };

    VkRenderPass pass = qoCreateRenderPass(t_device,
        .attachmentCount = 1,
        .pAttachments = (VkAttachmentDescription[]) {
            {
                QO_ATTACHMENT_DESCRIPTION_DEFAULTS,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            },
        },
        .subpassCount = 1,
        .pSubpasses = (VkSubpassDescription[]) {
            {
                QO_SUBPASS_DESCRIPTION_DEFAULTS,
                .colorAttachmentCount = 1,
                .pColorAttachments = (VkAttachmentReference[]) {
                    {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    },
                },
                .preserveAttachmentCount = 0,
            }
        },
        .pNext = &renderPassMultiviewCreateInfo
    );

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device);

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .taskShader = opts.task,
            .meshShader = mesh,
            .fragmentShader = fs,
            .pNext =
        &(VkGraphicsPipelineCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .renderPass = pass,
            .layout = pipeline_layout,
            .subpass = 0,
            .pViewportState = &(VkPipelineViewportStateCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &(VkViewport) { 0, 0, width, height, 0, 1 },
                .scissorCount = 1,
                .pScissors = &(VkRect2D) { { 0, 0 }, { width, height } },
            },
        }});

    VkImage image = qoCreateImage(t_device,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .mipLevels = 1,
        .arrayLayers = max_views,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1,
        });

    VkDeviceMemory image_mem = qoAllocImageMemory(t_device, image);
    qoBindImageMemory(t_device, image, image_mem, 0);

    VkImageView image_view = qoCreateImageView(t_device,
        QO_IMAGE_VIEW_CREATE_INFO_DEFAULTS,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .subresourceRange.layerCount = max_views
        );

    VkFramebuffer framebuffer = qoCreateFramebuffer(t_device,
        .renderPass = pass,
        .width = width,
        .height = height,
        .layers = 1,
        .attachmentCount = 1,
        .pAttachments = &image_view);

    vkCmdBeginRenderPass(t_cmd_buffer,
        &(VkRenderPassBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pass,
            .framebuffer = framebuffer,
            .renderArea = { { 0, 0 }, { width, height } },
            .clearValueCount = 1,
            .pClearValues = (VkClearValue[]) {
                { .color = { .float32 = {0.3, 0.3, 0.3, 1.0} } },
            }
        }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(t_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdDrawMeshTasksNV(t_cmd_buffer, 1, 0);

    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    test_result_t result = TEST_RESULT_PASS;

    for (int i = 0; i < max_views; i++) {
        string_t ref_name = STRING_INIT;

        if ((view_mask & (1 << i)) == 0)
            string_printf(&ref_name, "func.mesh.multiview.ref.empty.png");
        else
            string_printf(&ref_name, "%s.ref.%d.png", test_name, i);

        cru_image_t *ref = t_new_cru_image_from_filename(string_data(&ref_name));
        string_finish(&ref_name);

        cru_image_t *actual = t_new_cru_image_from_vk_image(t_device,
            t_queue, image, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_ASPECT_COLOR_BIT, width, height,
            /*miplevel*/ 0, /*array_slice*/i);

        t_dump_image_f(actual, "actual.%d.png", i);

        if (!cru_image_compare(actual, ref)) {
            loge("actual and reference images for view %d differ", i);
            result = TEST_RESULT_FAIL;

            string_t actual_name = STRING_INIT;
            string_printf(&actual_name, "%s.actual.%d.png", t_name, i);
            cru_image_write_file(actual, string_data(&actual_name));
            string_finish(&actual_name);
        }
    }

    return result;
}

static void
multiview(void)
{
    t_require_ext("VK_NV_mesh_shader");
    t_require_ext("VK_NVX_multiview_per_view_attributes");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        QO_EXTENSION GL_OVR_multiview : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) out vec4 color[];

        void main()
        {
            gl_PrimitiveCountNV = 2;

            gl_PrimitiveIndicesNV[0] = 0;
            gl_PrimitiveIndicesNV[1] = 1;
            gl_PrimitiveIndicesNV[2] = 2;
            gl_PrimitiveIndicesNV[3] = 3;
            gl_PrimitiveIndicesNV[4] = 4;
            gl_PrimitiveIndicesNV[5] = 5;

            for (int view_slot = 0; view_slot < gl_MeshViewCountNV; ++view_slot) {
                uint view_number = gl_MeshViewIndicesNV[view_slot];
                vec4 off = vec4(0.2 * view_number, 0.1 * view_number, 0, 0);

                gl_MeshVerticesNV[0].gl_PositionPerViewNV[view_slot] = vec4( 0.1f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[1].gl_PositionPerViewNV[view_slot] = vec4(-0.4f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[2].gl_PositionPerViewNV[view_slot] = vec4(-0.4f, -0.3f, 0.0f, 1.0f) + off;

                gl_MeshVerticesNV[3].gl_PositionPerViewNV[view_slot] = vec4( 0.1f, -0.3f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[4].gl_PositionPerViewNV[view_slot] = vec4( 0.1f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[5].gl_PositionPerViewNV[view_slot] = vec4(-0.4f, -0.3f, 0.0f, 1.0f) + off;
            }

            color[0] = vec4(0, 1, 0, 1);
            color[1] = vec4(0, 1, 0, 1);
            color[2] = vec4(0, 1, 0, 1);
            color[3] = vec4(0, 0, 1, 1);
            color[4] = vec4(0, 0, 1, 1);
            color[5] = vec4(0, 0, 1, 1);
        }
    );

    test_result_t result = run_multiview_mesh_pipeline(mesh, NULL);

    if (result != TEST_RESULT_PASS)
        t_end(result);
}

test_define {
    .name = "func.mesh.multiview.3.111",
    .start = multiview,
    .no_image = true,
};

test_define {
    .name = "func.mesh.multiview.3.110",
    .start = multiview,
    .no_image = true,
};

test_define {
    .name = "func.mesh.multiview.2.10",
    .start = multiview,
    .no_image = true,
};

test_define {
    .name = "func.mesh.multiview.2.11",
    .start = multiview,
    .no_image = true,
};

test_define {
    .name = "func.mesh.multiview.1.1",
    .start = multiview,
    .no_image = true,
};

static void
multiview_perview_nonblock(void)
{
    t_require_ext("VK_NV_mesh_shader");
    t_require_ext("VK_NVX_multiview_per_view_attributes");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_NV_mesh_shader : require
        QO_EXTENSION GL_OVR_multiview : require
        layout(local_size_x = 1) in;
        layout(max_vertices = 6) out;
        layout(max_primitives = 3) out;
        layout(triangles) out;

        layout(location = 0) perviewNV out vec4 color[][];

        void main()
        {
            gl_PrimitiveCountNV = 2;

            gl_PrimitiveIndicesNV[0] = 0;
            gl_PrimitiveIndicesNV[1] = 1;
            gl_PrimitiveIndicesNV[2] = 2;
            gl_PrimitiveIndicesNV[3] = 3;
            gl_PrimitiveIndicesNV[4] = 4;
            gl_PrimitiveIndicesNV[5] = 5;

            for (int view_slot = 0; view_slot < gl_MeshViewCountNV; ++view_slot) {
                uint view_number = gl_MeshViewIndicesNV[view_slot];
                vec4 off = vec4(0.2 * view_number, 0.1 * view_number, 0, 0);

                gl_MeshVerticesNV[0].gl_PositionPerViewNV[view_slot] = vec4( 0.1f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[1].gl_PositionPerViewNV[view_slot] = vec4(-0.4f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[2].gl_PositionPerViewNV[view_slot] = vec4(-0.4f, -0.3f, 0.0f, 1.0f) + off;

                gl_MeshVerticesNV[3].gl_PositionPerViewNV[view_slot] = vec4( 0.1f, -0.3f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[4].gl_PositionPerViewNV[view_slot] = vec4( 0.1f,  0.2f, 0.0f, 1.0f) + off;
                gl_MeshVerticesNV[5].gl_PositionPerViewNV[view_slot] = vec4(-0.4f, -0.3f, 0.0f, 1.0f) + off;

                if (view_number == 0) {
                    color[0][view_slot] = vec4(0, 1, 0, 1);
                    color[1][view_slot] = vec4(0, 1, 0, 1);
                    color[2][view_slot] = vec4(0, 1, 0, 1);
                    color[3][view_slot] = vec4(0, 0, 1, 1);
                    color[4][view_slot] = vec4(0, 0, 1, 1);
                    color[5][view_slot] = vec4(0, 0, 1, 1);
                } else if (view_number == 1) {
                    color[0][view_slot] = vec4(1, 0, 0, 1);
                    color[1][view_slot] = vec4(1, 0, 0, 1);
                    color[2][view_slot] = vec4(1, 0, 0, 1);
                    color[3][view_slot] = vec4(1, 1, 1, 1);
                    color[4][view_slot] = vec4(1, 1, 1, 1);
                    color[5][view_slot] = vec4(1, 1, 1, 1);
                } else { // should be impossible
                    color[0][view_slot] = vec4(1, 1, 0, 1);
                    color[1][view_slot] = vec4(1, 1, 0, 1);
                    color[2][view_slot] = vec4(1, 1, 0, 1);
                    color[3][view_slot] = vec4(0, 1, 1, 1);
                    color[4][view_slot] = vec4(0, 1, 1, 1);
                    color[5][view_slot] = vec4(0, 1, 1, 1);
                }
            }
        }
    );

    test_result_t result = run_multiview_mesh_pipeline(mesh, NULL);

    if (result != TEST_RESULT_PASS)
        t_end(result);
}

test_define {
    .name = "func.mesh.multiview.perview.nonblock.2.11",
    .start = multiview_perview_nonblock,
    .no_image = true,
};
