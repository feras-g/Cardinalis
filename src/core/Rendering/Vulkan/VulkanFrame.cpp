#include "VulkanFrame.h"

void VulkanFrame::Destroy(VkDevice device)
{
	vkDestroyCommandPool(device, cmdPool, nullptr);
	vkDestroyFence(device, renderFence, nullptr);
	vkDestroySemaphore(device, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(device, queueSubmittedSemaphore, nullptr);
}
