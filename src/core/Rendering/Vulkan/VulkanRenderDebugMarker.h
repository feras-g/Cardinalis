#ifndef VULKAN_RENDER_DEBUG_MARKER
#define VULKAN_RENDER_DEBUG_MARKER

#define VULKAN_RENDER_DEBUG_MARKER(...) VulkanRenderDebugMarker(__VA_ARGS__)

// Helper over Vulkan debug marker using RAII to mark a scope

#include "Rendering/Vulkan/VulkanRenderInterface.h"

class VulkanRenderDebugMarker
{
public:
	VulkanRenderDebugMarker(VkCommandBuffer cmdBuffer, const char* markerName);

	~VulkanRenderDebugMarker();
private:
	// Function pointers
	PFN_vkCmdDebugMarkerBeginEXT		fpCmdDebugMarkerBeginEXT		= nullptr;
	PFN_vkCmdDebugMarkerEndEXT			fpCmdDebugMarkerEndEXT			= nullptr;
	PFN_vkDebugMarkerSetObjectNameEXT	fpDebugMarkerSetObjectNameEXT	= nullptr;

	static bool initialized;

	const char* markerName = nullptr;
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
};

#endif // !VULKAN_RENDER_DEBUG_MARKER
