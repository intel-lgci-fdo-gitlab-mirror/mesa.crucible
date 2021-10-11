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

