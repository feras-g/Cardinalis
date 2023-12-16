#pragma once

#include "core/rendering/vulkan/VulkanRenderInterface.h"

/************************************************************************************************/
/* Debug marker utility class using RAII to mark a scope										*/
/* https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html	*/
/************************************************************************************************/
class VulkanRenderDebugMarker
{
public:
	VulkanRenderDebugMarker(VkCommandBuffer cmd_buffer, const char* name, std::array<float, 4> color = { 1.0f, 1.0f, 1.0f, 1.0f })
	{
#if ENGINE_DEBUG
		//this->m_cmd_buffer = cmd_buffer;
		//VkDebugUtilsLabelEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, name, *color.data() };
		//fpCmdBeginDebugUtilsLabelEXT(m_cmd_buffer, &info);
#endif // ENGINE_DEBUG
	}

	~VulkanRenderDebugMarker()
	{
#if ENGINE_DEBUG
		//fpCmdEndDebugUtilsLabelEXT(m_cmd_buffer);
#endif // ENGINE_DEBUG
	}
private:
	const char* m_name = nullptr;
	VkCommandBuffer m_cmd_buffer = VK_NULL_HANDLE;
};

#define VULKAN_RENDER_DEBUG_MARKER(...) VulkanRenderDebugMarker a = VulkanRenderDebugMarker(__VA_ARGS__);

/*
*	Set a debug name for a Vulkan object
*/
static void set_object_name(VkObjectType obj_type, uint64_t obj_handle, const char* obj_name)
{
#if ENGINE_DEBUG
	//const VkDebugUtilsObjectNameInfoEXT debug_info = 
	//{
	//	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	//	.objectType = obj_type,
	//	.objectHandle = obj_handle,
	//	.pObjectName = obj_name,
	//};

	//fpSetDebugUtilsObjectNameEXT(ctx.device, &debug_info);
#endif // ENGINE_DEBUG
}

struct EngineUtils
{
	static inline size_t round_to(size_t size, size_t alignment)
	{
		size_t sz = (size + alignment);
		return sz - (sz % alignment);
	}

	static inline uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties)
	{
		VkPhysicalDeviceMemoryProperties deviceMemProperties;
		vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemProperties);

		for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
		{
			if ((memoryTypeBits & (1 << i) && (deviceMemProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties))
			{
				return i;
			}
		}

		return 0;
	}

};

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

/* Assert on VkResult */
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

static void DEBUG_INSERT_FULL_PIPELINE_BARRIER(VkCommandBuffer cmd_buffer)
{
#if ENGINE_DEBUG
	VkMemoryBarrier2 memoryBarrier = {
	  .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	  .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
	  .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR |
					   VK_ACCESS_2_MEMORY_WRITE_BIT_KHR,
	  .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
	  .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR |
					   VK_ACCESS_2_MEMORY_WRITE_BIT_KHR };

	VkDependencyInfo dependencyInfo = {
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
		nullptr,
		0,
		1,              // memoryBarrierCount
		&memoryBarrier, // pMemoryBarriers
	};

	vkCmdPipelineBarrier2(cmd_buffer, &dependencyInfo);
#endif // ENGINE_DEBUG
}