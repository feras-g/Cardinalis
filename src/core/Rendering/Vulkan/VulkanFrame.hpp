#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrame
{
	VkFence fence_queue_submitted;
	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_buffer;
	VkSemaphore semaphore_swapchain_acquire;
	VkSemaphore smp_queue_submitted;
};

static void destroy(VkDevice device, VulkanFrame frame)
{
	vkDestroyCommandPool(device, frame.cmd_pool, nullptr);
	vkDestroyFence(device, frame.fence_queue_submitted, nullptr);
	vkDestroySemaphore(device, frame.semaphore_swapchain_acquire, nullptr);
	vkDestroySemaphore(device, frame.smp_queue_submitted, nullptr);
}
