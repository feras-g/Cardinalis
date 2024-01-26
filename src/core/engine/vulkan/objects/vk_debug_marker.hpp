#pragma once
#include "core/engine/vulkan/vk_context.h"
#include "core/engine/vulkan/objects/vk_device.h"

namespace vk
{
	/************************************************************************************************/
	/* Debug marker utility class using RAII to mark a scope										*/
	/* https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html	*/
	/************************************************************************************************/
	class debug_marker
	{
	public:
		debug_marker(VkCommandBuffer cmd_buffer, const char* name, std::array<float, 4> color = { 1.0f, 1.0f, 1.0f, 1.0f })
		{
			this->m_cmd_buffer = cmd_buffer;
			VkDebugUtilsLabelEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, name, *color.data() };
			fpCmdBeginDebugUtilsLabelEXT(m_cmd_buffer, &info);
		}

		~debug_marker()
		{
			fpCmdEndDebugUtilsLabelEXT(m_cmd_buffer);
		}
	private:
		const char* m_name = nullptr;
		VkCommandBuffer m_cmd_buffer = VK_NULL_HANDLE;
	};

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

		fpSetDebugUtilsObjectNameEXT(ctx.device, &debug_info);
	}
}

#define VULKAN_RENDER_DEBUG_MARKER(...) vk::debug_marker marker = vk::debug_marker(__VA_ARGS__);