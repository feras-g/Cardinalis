#ifndef VULKAN_SWAPCHAIN_H
#define	VULKAN_SWAPCHAIN_H

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include <vector>
#include <vulkan/vulkan.h>
class VulkanTexture;

struct VulkanSwapchainMetadata
{
	uint32_t imageCount;
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkExtent2D extent;
	VkPresentModeKHR presentMode;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain() = default;
	VulkanSwapchain(VkSurfaceKHR surface, VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice);
	void Initialize(VkFormat format, VkColorSpaceKHR colorSpace);
	void Reinitialize(VkFormat format, VkColorSpaceKHR colorSpace);

	// Reset metadata and destroy images and framebuffers
	void Destroy();
	void AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* pImageIndex) const;
	void Present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices);

	inline VkSwapchainKHR Get() const;

	// Function pointers
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR		fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	fpGetPhysicalDeviceSurfacePresentModesKHR;

	PFN_vkCreateSwapchainKHR	fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR	fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR	fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR		fpQueuePresentKHR;

	VkSwapchainKHR swapchain;
	VulkanSwapchainMetadata metadata;

	// Data
	std::vector<VulkanTexture> images;
	std::vector<VkFramebuffer> framebuffers;

protected:
	// Vulkans context handles 
	VkSurfaceKHR hSurface;
	VkInstance hVkInstance;
	VkDevice hDevice;
	VkPhysicalDevice hPhysicalDevice;
};

#endif // !VULKAN_SWAPCHAIN_H
