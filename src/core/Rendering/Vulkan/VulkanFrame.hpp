#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrame
{
	VkSemaphore imageAcquiredSemaphore	 = VK_NULL_HANDLE;
	VkSemaphore queueSubmittedSemaphore	 = VK_NULL_HANDLE;
	VkFence renderFence					 = VK_NULL_HANDLE;
								 
	VkCommandPool cmd_pool		 = VK_NULL_HANDLE;
	VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
};

static void Destroy(VkDevice device, VulkanFrame frame)
{
	vkDestroyCommandPool(device, frame.cmd_pool, nullptr);
	vkDestroyFence(device, frame.renderFence, nullptr);
	vkDestroySemaphore(device, frame.imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(device, frame.queueSubmittedSemaphore, nullptr);
}
