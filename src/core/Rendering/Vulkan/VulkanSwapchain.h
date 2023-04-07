#ifndef VULKAN_SWAPCHAIN_H
#define	VULKAN_SWAPCHAIN_H

#include <vector>
#include <vulkan/vulkan.h>

struct Texture2D;

struct VulkanSwapchainInfo
{
	uint32_t imageCount;
	VkFormat colorFormat;
	VkFormat depthStencilFormat;
	VkColorSpaceKHR colorSpace;
	VkExtent2D extent;
	VkPresentModeKHR presentMode;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain() = default;
	VulkanSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physDevice);
	void Initialize(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat);
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
