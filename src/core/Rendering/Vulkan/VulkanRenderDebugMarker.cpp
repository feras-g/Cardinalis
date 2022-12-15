#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

bool VulkanRenderDebugMarker::initialized = false;

VulkanRenderDebugMarker::VulkanRenderDebugMarker(VkCommandBuffer cmdBuffer, const char* markerName) 
{
	this->cmdBuffer = cmdBuffer;

	//if (!initialized)
	{
		fpCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdInsertDebugUtilsLabelEXT");
		fpCmdBeginDebugUtilsLabelEXT  = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdBeginDebugUtilsLabelEXT");
		fpCmdEndDebugUtilsLabelEXT    = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdEndDebugUtilsLabelEXT");

		initialized = true;
	}

	VkDebugUtilsLabelEXT  info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, markerName, {0.55f, 0.33f,0.33f,1.0f} };
	fpCmdBeginDebugUtilsLabelEXT(cmdBuffer, &info);
};


VulkanRenderDebugMarker::~VulkanRenderDebugMarker()
{
	fpCmdEndDebugUtilsLabelEXT(cmdBuffer);
};