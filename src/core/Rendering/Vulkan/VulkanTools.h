#pragma once
#include <string>
#include <sstream>
#include "Core/EngineLogger.h"
#include <vulkan/vulkan.h>

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

uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties);

using ImageData = std::unique_ptr<unsigned char, decltype(&free)>;
struct Image
{
    ImageData data;
    int w{}, h{}, n{};
    ~Image() { data = nullptr; }
};

Image load_image_from_file(std::string_view filename);

#define CHECK_DEREF(p) { assert(p); return *p; }
