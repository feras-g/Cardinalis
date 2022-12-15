#ifndef VULKAN_RENDER_DEBUG_MARKER
#define VULKAN_RENDER_DEBUG_MARKER

#define VULKAN_RENDER_DEBUG_MARKER(...) VulkanRenderDebugMarker a = VulkanRenderDebugMarker(__VA_ARGS__);

// Helper over Vulkan debug marker using RAII to mark a scope
// Examples: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html

#include "Rendering/Vulkan/VulkanRenderInterface.h"

class VulkanRenderDebugMarker
{
public:
	VulkanRenderDebugMarker(VkCommandBuffer cmdBuffer, const char* markerName);

	~VulkanRenderDebugMarker();
private:
	// Function pointers
	PFN_vkCmdBeginDebugUtilsLabelEXT	fpCmdBeginDebugUtilsLabelEXT	= nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT		fpCmdEndDebugUtilsLabelEXT      = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT	fpCmdInsertDebugUtilsLabelEXT   = nullptr;

	static bool initialized;

	const char* markerName = nullptr;
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
};

#endif // !VULKAN_RENDER_DEBUG_MARKER
