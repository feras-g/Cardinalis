#pragma once

#include <string>
#include <sstream>
#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"

#include "stb_image.h"

static constexpr uint64_t OneSecondInNanoSeconds = 1000000000;

#define VK_CHECK(x)                                                                             \
	do                                                                                          \
	{                                                                                           \
		VkResult err = x;                                                                       \
		if (err)                                                                                \
		{                                                                                       \
			LOG_ERROR("Detected Vulkan error {0} at {1}:{2}.\n", string_VkResult(err), __FILE__, __LINE__); \
			abort();                                                                            \
		}                                                                                       \
	} while (0)

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

using ImageData = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
struct Image
{
    ImageData data;
    int w{}, h{}, n{};
    ~Image() { data = nullptr; }
};

static Image load_image_from_file(std::string_view filename)
{
    int w, h, n;
    stbi_uc* data = stbi_load(filename.data(), &w, &h, &n, 0);
    assert(data);
    return { ImageData(data, free), w, h, n };
}

#define CHECK_DEREF(p) { assert(p); return *p; }
