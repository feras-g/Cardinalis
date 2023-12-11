#pragma once

#include "core/engine/vulkan/vk_common.h"
#include "core/engine/vulkan/objects/vk_command_buffer.h"

namespace vk
{
	struct frame
	{
		VkFence fence_queue_submitted;
		command_buffer cmd_buffer;
		VkSemaphore semaphore_swapchain_acquire;
		VkSemaphore smp_queue_submitted;
	};

	static void destroy(VkDevice device, frame frame)
	{
		vkDestroyFence(device, frame.fence_queue_submitted, nullptr);
		vkDestroySemaphore(device, frame.semaphore_swapchain_acquire, nullptr);
		vkDestroySemaphore(device, frame.smp_queue_submitted, nullptr);
	}
}

