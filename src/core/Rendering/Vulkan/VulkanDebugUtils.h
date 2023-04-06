#pragma once

#include "Rendering/Vulkan/VulkanRenderInterface.h"

/************************************************************************************************/
/* Debug marker utility class using RAII to mark a scope										*/
/* https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html	*/
/************************************************************************************************/
class VulkanRenderDebugMarker
{
public:
	VulkanRenderDebugMarker(VkCommandBuffer cmd_buffer, const char* name, std::array<float, 4> color = { 1.0f, 1.0f, 1.0f, 1.0f })
	{
		this->m_cmd_buffer = cmd_buffer;
		VkDebugUtilsLabelEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, name, *color.data() };
		fpCmdBeginDebugUtilsLabelEXT(m_cmd_buffer, &info);
	}

	~VulkanRenderDebugMarker()
	{
		fpCmdEndDebugUtilsLabelEXT(m_cmd_buffer);
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
	const VkDebugUtilsObjectNameInfoEXT debug_info = 
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = obj_type,
		.objectHandle = obj_handle,
		.pObjectName = obj_name,
	};

	fpSetDebugUtilsObjectNameEXT(context.device, &debug_info);
}
