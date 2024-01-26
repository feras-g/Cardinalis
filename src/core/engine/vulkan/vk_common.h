#pragma once

#include <vulkan/vulkan.h>

#if ENGINE_DEBUG
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(x)                                                                             \
	do                                                                                          \
	{                                                                                           \
		VkResult err = x;                                                                       \
		if (err)                                                                                \
		{                                                                                       \
			LOG_ERROR("Detected Vulkan error {0} at {1}:{2}.\n", vk_object_to_string(err), __FILE__, __LINE__); \
			abort();                                                                            \
		}                                                                                       \
	} while (0)

/*
*	Return the value of a VkObject as a string
*/
inline const char* vk_object_to_string(VkResult input_value)
{
	return string_VkResult(input_value);
}

inline const char* vk_object_to_string(VkPresentModeKHR input_value)
{
	return string_VkPresentModeKHR(input_value);
}

inline const char* vk_object_to_string(VkShaderStageFlagBits input_value)
{
	return string_VkShaderStageFlagBits(input_value);
}

inline const char* vk_object_to_string(VkPhysicalDeviceType input_value)
{
	return string_VkPhysicalDeviceType(input_value);
}

#else
#define VK_CHECK(x) assert(x == VK_SUCCESS);
#endif

static constexpr uint32_t vk_api_version = VK_MAKE_VERSION(1, 3, 1);

static inline size_t round_to(size_t value, size_t alignment)
{
	size_t val = (value + alignment);
	return val - (val % alignment);
}
