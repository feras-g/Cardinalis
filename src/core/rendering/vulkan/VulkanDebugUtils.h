#pragma once

#include "core/engine/vulkan/objects/vk_device.h"




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