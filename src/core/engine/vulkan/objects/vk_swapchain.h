#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct Texture2D;

namespace vk
{
	struct swapchain_info
	{
		uint32_t image_count;
		uint32_t width;
		uint32_t height;
		VkFormat color_format;
		VkFormat depth_format;
		VkColorSpaceKHR color_space;
		VkPresentModeKHR present_mode;
	};

	class swapchain
	{
	public:
		swapchain() = default;
		swapchain(VkSurfaceKHR surface);
		void init(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat);
		void reinitialize();
		void destroy();
		void clear_color(VkCommandBuffer cmd_buffer);

		uint32_t current_backbuffer_idx = 0;

		VkResult acquire_next_image(VkSemaphore imageAcquiredSmp);
		VkResult present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices);

		VkSurfaceKHR h_surface;
		VkSwapchainKHR vk_swapchain;
		swapchain_info info;

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
}

