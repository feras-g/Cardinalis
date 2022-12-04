#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

bool VulkanRenderDebugMarker::initialized = false;

VulkanRenderDebugMarker::VulkanRenderDebugMarker(VkCommandBuffer cmdBuffer, const char* markerName) 
{
	this->cmdBuffer = cmdBuffer;

	if (!initialized)
	{
		GET_DEVICE_PROC_ADDR(context.device, DebugMarkerSetObjectNameEXT);
		GET_DEVICE_PROC_ADDR(context.device, CmdDebugMarkerBeginEXT);
		GET_DEVICE_PROC_ADDR(context.device, CmdDebugMarkerEndEXT);

		VulkanRenderDebugMarker::initialized = true;
	}

	VkDebugMarkerObjectNameInfoEXT objectInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT, .pObjectName = markerName };
	fpDebugMarkerSetObjectNameEXT(context.device, &objectInfo);

	VkDebugMarkerMarkerInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT, nullptr, markerName, {1.0f,0.0f,0.0f,1.0f} };
	fpCmdDebugMarkerBeginEXT(cmdBuffer, &info);
};


VulkanRenderDebugMarker::~VulkanRenderDebugMarker()
{
	fpCmdDebugMarkerEndEXT(cmdBuffer);
};