#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct Texture2D;

struct VulkanSwapchainInfo
{
	uint32_t image_count;
	uint32_t width;
	uint32_t height;
	VkFormat color_format;
	VkFormat depth_format;
	VkColorSpaceKHR color_space;
	VkPresentModeKHR present_mode;
};

class VulkanSwapchain
{
public:
	VulkanSwapchain() = default;
	VulkanSwapchain(VkSurfaceKHR surface);
	void init(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat);
	void reinitialize();
	void destroy();

	uint32_t current_backbuffer_idx = 0;

	VkResult acquire_next_image(VkSemaphore imageAcquiredSmp);
	VkResult present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices);
	
	VkSurfaceKHR h_surface;
	VkSwapchainKHR vk_swapchain;
	VulkanSwapchainInfo info;

	std::vector<Texture2D> color_attachments;
	std::vector<Texture2D> depth_attachments;
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
