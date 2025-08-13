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

#include "tapi/t.h"
#include "util/string.h"

#include "src/tests/func/mesh/ext/layer-spirv.h"

#define GET_DEVICE_FUNCTION_PTR(name) \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(t_device, #name); \
    t_assert(name != NULL);

typedef struct layer_mesh_pipeline_options layer_mesh_pipeline_options_t;

struct layer_mesh_pipeline_options {
    VkShaderModule fs;
};

static test_result_t
run_layer_mesh_pipeline(VkShaderModule mesh,
                        const struct layer_mesh_pipeline_options *_opts)
{
    t_require_ext("VK_EXT_mesh_shader");

    const unsigned width = 128;
    const unsigned height = 128;

    VkPhysicalDeviceMeshShaderFeaturesEXT features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
    };
    VkPhysicalDeviceFeatures2 pfeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features,
    };
    vkGetPhysicalDeviceFeatures2(t_physical_dev, &pfeatures);

    struct layer_mesh_pipeline_options opts = *_opts;

    if (!features.meshShader)
        t_skipf("meshShader not supported");

    GET_DEVICE_FUNCTION_PTR(vkCmdDrawMeshTasksEXT);

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
        }
    );

    VkPipelineLayout pipeline_layout = qoCreatePipelineLayout(t_device,
        .setLayoutCount = 0,
        .pSetLayouts = NULL
    );

    VkPipeline pipeline = qoCreateGraphicsPipeline(t_device, t_pipeline_cache,
        &(QoExtraGraphicsPipelineCreateInfo) {
            QO_EXTRA_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULTS,
            .meshShader = mesh,
            .fragmentShader = opts.fs,
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
        .arrayLayers = 4,
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
        .subresourceRange.layerCount = 4
        );

    VkFramebuffer framebuffer = qoCreateFramebuffer(t_device,
        .renderPass = pass,
        .width = width,
        .height = height,
        .layers = 4,
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

    vkCmdDrawMeshTasksEXT(t_cmd_buffer, 1, 1, 1);

    vkCmdEndRenderPass(t_cmd_buffer);
    qoEndCommandBuffer(t_cmd_buffer);
    qoQueueSubmit(t_queue, 1, &t_cmd_buffer, VK_NULL_HANDLE);
    qoQueueWaitIdle(t_queue);

    test_result_t result = TEST_RESULT_PASS;

    for (int i = 0; i < 4; i++) {
        string_t ref_name = STRING_INIT;

        string_printf(&ref_name, "func.mesh.layer.ref.%d.png", i);

        cru_image_t *ref = t_new_cru_image_from_filename(string_data(&ref_name));
        string_finish(&ref_name);

        cru_image_t *actual = t_new_cru_image_from_vk_image(t_device,
            t_queue, image, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_ASPECT_COLOR_BIT, width, height,
            /*miplevel*/ 0, /*array_slice*/i);

        t_dump_image_f(actual, "actual.%d.png", i);

        if (!cru_image_compare(actual, ref)) {
            loge("actual and reference images for layer %d differ", i);
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
layer(void)
{
    t_require_ext("VK_EXT_mesh_shader");

    VkShaderModule mesh = qoCreateShaderModuleGLSL(t_device, MESH,
        QO_EXTENSION GL_EXT_mesh_shader : require
        QO_TARGET_ENV spirv1.4
        layout(local_size_x = 1) in;
        layout(max_vertices = 12) out;
        layout(max_primitives = 4) out;
        layout(triangles) out;

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

            gl_MeshPrimitivesEXT[0].gl_Layer = 0;
            gl_MeshPrimitivesEXT[1].gl_Layer = 1;
            gl_MeshPrimitivesEXT[2].gl_Layer = 2;
            gl_MeshPrimitivesEXT[3].gl_Layer = 3;
        }
    );

    VkShaderModule fs = qoCreateShaderModuleGLSL(t_device, FRAGMENT,
        layout(location = 0) out vec4 f_color;

        void main()
        {
            switch (gl_Layer) {
            case 0:
                f_color = vec4(1, 0, 0, 1);
                break;
            case 1:
                f_color = vec4(0, 1, 0, 1);
                break;
            case 2:
                f_color = vec4(0, 0, 1, 1);
                break;
            case 3:
                f_color = vec4(1, 1, 1, 1);
                break;
            default:
                f_color = vec4(0, 0, 0, 1);
                break;
            }
        }
    );

    layer_mesh_pipeline_options_t opts = {
            .fs = fs,
    };

    test_result_t result = run_layer_mesh_pipeline(mesh, &opts);

    if (result != TEST_RESULT_PASS)
        t_end(result);
}

test_define {
    .name = "func.mesh.ext.layer",
    .start = layer,
    .no_image = true,
    .mesh_shader = true,
};
