// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <math.h>
#include "util/simple_pipeline.h"
#include "tapi/t.h"

// \file
// Reproduce an Intel anv bug from mesa#15578.
//
// Wrong expectation about dedicated image for allocations.

static void
test(void)
{
    t_require_ext("VK_KHR_external_memory");
    t_require_ext("VK_KHR_external_memory_fd");

    VkExternalMemoryHandleTypeFlagBits handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

#define GET_FUNCTION_PTR(name, device) \
    PFN_vk##name name = (PFN_vk##name)vkGetDeviceProcAddr(device, "vk"#name)

    GET_FUNCTION_PTR(GetMemoryFdKHR, t_device);

#undef GET_FUNCTION_PTR

    VkMemoryRequirements reqs = {
      .size = 4096,
      .alignment = 4096,
      .memoryTypeBits = 0x1,
    };

    VkDeviceMemory mem1 = qoAllocMemoryFromRequirements(t_device,
                                                        &reqs,
                                                        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                        .pNext = &(VkExportMemoryAllocateInfo) {
                                                          .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
                                                          .handleTypes = handle_type,
                                                        });

    int fd;
    VkResult result = GetMemoryFdKHR(t_device,
                                     &(VkMemoryGetFdInfoKHR) {
                                       .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
                                       .memory = mem1,
                                       .handleType = handle_type,
                                     }, &fd);
    t_assert(result == VK_SUCCESS);
    t_assert(fd >= 0);

    qoAllocMemoryFromRequirements(t_device,
                                  &reqs,
                                  .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  .pNext = &(VkImportMemoryFdInfoKHR) {
                                    .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
                                    .handleType = handle_type,
                                    .fd = fd,
                                  });


    t_pass();
}

test_define {
    .name = "bug.gitlab-15578",
    .start = test,
    .no_image = true,
};
