#ifndef VULKAN_SWAPCHAIN_H
#define	VULKAN_SWAPCHAIN_H

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanTexture;

struct VulkanSwapchainInfo
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
	VulkanSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physDevice);
	void Initialize(VkFormat format, VkColorSpaceKHR colorSpace);
	void Reinitialize(VkFormat format, VkColorSpaceKHR colorSpace);

	// Reset metadata and destroy images and framebuffers
	void Destroy();
	void AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* pImageIndex) const;
	void Present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices);

	inline VkSwapchainKHR Get() const;

	VkPhysicalDevice hPhysicalDevice;
	VkSurfaceKHR hSurface;
	VkSwapchainKHR swapchain;
	VulkanSwapchainInfo info;

	// Data
	std::vector<VulkanTexture> images;
	std::vector<VkFramebuffer> framebuffers;
private:
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
};

#endif // !VULKAN_SWAPCHAIN_H
