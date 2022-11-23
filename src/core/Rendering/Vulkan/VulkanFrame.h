#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrame
{
	VkSemaphore imageAcquiredSemaphore	 = VK_NULL_HANDLE;
	VkSemaphore renderCompleteSemaphore	 = VK_NULL_HANDLE;
	VkFence renderFence					 = VK_NULL_HANDLE;
								 
	VkCommandPool cmdPool		 = VK_NULL_HANDLE;
	VkCommandBuffer cmdBuffer	 = VK_NULL_HANDLE;

	void Destroy(VkDevice device);
};