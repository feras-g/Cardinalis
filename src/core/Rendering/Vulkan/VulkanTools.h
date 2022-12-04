#ifndef VULKAN_TOOLS_H
#define VULKAN_TOOLS_H

#include <string>
#include <sstream>
#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"

static constexpr uint64_t OneSecondInNanoSeconds = 1000000000;

#define VK_CHECK(x)													                                \
if(x != VK_SUCCESS) { LOG_ERROR("Assert on VkResult : {0}", string_VkResult(x)); assert(false); };  \

#define GET_INSTANCE_PROC_ADDR(inst, entry)                            \
{                                                                      \
  fp##entry = (PFN_vk##entry)vkGetInstanceProcAddr(inst, "vk" #entry); \
  if (!fp##entry)                                                      \
    EXIT_ON_ERROR("vkGetInstanceProcAddr failed to find vk" #entry);   \
}

#define GET_DEVICE_PROC_ADDR(dev, entry)                            \
{                                                                   \
  fp##entry = (PFN_vk##entry)vkGetDeviceProcAddr(dev, "vk" #entry); \
  if (!fp##entry)                                                   \
    EXIT_ON_ERROR("vkGetDeviceProcAddr failed to find vk" #entry);  \
}

static uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties)
{
    // Query available types of memory
    VkPhysicalDeviceMemoryProperties deviceMemProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemProperties);

    // Find the most suitable memory type 
    for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
    {
        if ((memoryTypeBits & (1 << i) && (deviceMemProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties))
        {
            return i;
        }
    }

    return 0;
}

#endif // !VULKAN_TOOLS_H