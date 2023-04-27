#pragma once

#include <vulkan/vulkan.h>

struct VulkanFrame
{
	VkSemaphore smp_image_acquired	 = VK_NULL_HANDLE;
	VkSemaphore smp_queue_submitted	 = VK_NULL_HANDLE;
	VkFence fence_queue_submitted					 = VK_NULL_HANDLE;
								 
	VkCommandPool cmd_pool		 = VK_NULL_HANDLE;
	VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
};

static void Destroy(VkDevice device, VulkanFrame frame)
{
	vkDestroyCommandPool(device, frame.cmd_pool, nullptr);
	vkDestroyFence(device, frame.fence_queue_submitted, nullptr);
	vkDestroySemaphore(device, frame.smp_image_acquired, nullptr);
	vkDestroySemaphore(device, frame.smp_queue_submitted, nullptr);
}
