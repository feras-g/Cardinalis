#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct Texture2D;

struct VulkanSwapchainInfo
{
	uint32_t imageCount;
	VkFormat color_format;
	VkFormat depth_format;
	VkColorSpaceKHR colorSpace;
	VkExtent2D extent;
	VkPresentModeKHR presentMode;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain() = default;
	VulkanSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physDevice);
	void init(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat);
	void Reinitialize();

	// Reset metadata and destroy images and framebuffers
	void Destroy();
	VkResult AcquireNextImage(VkSemaphore imageAcquiredSmp, uint32_t* pImageIndex) const;
	void Present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices);

	inline VkSwapchainKHR Get() const;
	
	VkPhysicalDevice hPhysicalDevice;
	VkSurfaceKHR hSurface;
	VkSwapchainKHR swapchain;
	VulkanSwapchainInfo info;

	// Data
	std::vector<Texture2D> color_attachments;
	std::vector<Texture2D> depthTextures;
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
