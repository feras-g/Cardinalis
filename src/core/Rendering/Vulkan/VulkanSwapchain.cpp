#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanRenderInterface.h"
#include "VulkanDebugUtils.h"
#include "RenderPass.h"

VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface)
    : h_surface(surface)
{
    const VkInstance& instance = context.device.instance;
    const VkDevice& device = context.device;

    // Initialize function pointers
	fpGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
	fpGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	fpGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	fpGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

	fpCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
	fpDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
	fpGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
	fpAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
	fpQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");

    // Present mode
    uint32_t presentModeCount = 0;
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(context.device.physical_device, h_surface, &presentModeCount, nullptr));
    assert(presentModeCount >= 1);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(context.device.physical_device, h_surface, &presentModeCount, presentModes.data()));
	info.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    LOG_WARN("Current present mode : {0}", vk_object_to_string(info.present_mode));
}

void VulkanSwapchain::init(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat)
{
    // Query/Set surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(context.device.physical_device, h_surface, &caps));

    LOG_INFO("Surface capabilities : minImageCount={0} maxImageCount={1}, currentImageExtent={2}x{3}, maxImageExtent={4}x{2}\n",
        caps.minImageCount, caps.maxImageCount, caps.currentExtent.width, caps.currentExtent.height,
        caps.maxImageExtent.width, caps.maxImageExtent.height);

    uint32_t surfaceFormatsCount = 0;
    fpGetPhysicalDeviceSurfaceFormatsKHR(context.device.physical_device, h_surface, &surfaceFormatsCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats (surfaceFormatsCount);
    fpGetPhysicalDeviceSurfaceFormatsKHR(context.device.physical_device, h_surface, &surfaceFormatsCount, surfaceFormats.data());

   
    assert(caps.maxImageCount >= 1);
    info.width = caps.currentExtent.width;
    info.height = caps.currentExtent.height;

    info.image_count = NUM_FRAMES;// std::min(NUM_FRAMES, caps.maxImageCount);

    info.color_format = colorFormat;
    info.color_space = colorSpace;
    info.depth_format = depthStencilFormat;// VK_FORMAT_D32_SFLOAT;

    // Swapchain creation
    VkSwapchainCreateInfoKHR swapchainCreateInfo =
    {
            .sType= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .flags=NULL,
            .surface=h_surface,
            .minImageCount= info.image_count,
            .imageFormat= info.color_format,
            .imageColorSpace= info.color_space,
            .imageExtent = { info.width, info.height },
            .imageArrayLayers=1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode= VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount=1,
            .pQueueFamilyIndices={0},
            .preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode= info.present_mode,
            .clipped=VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
    };

    VK_CHECK(vkCreateSwapchainKHR(context.device, &swapchainCreateInfo, nullptr, &vk_swapchain));
    
    depth_attachments.resize(info.image_count);
    color_attachments.resize(info.image_count);

    std::vector<VkImage> tmp(info.image_count);
    VK_CHECK(fpGetSwapchainImagesKHR(context.device, vk_swapchain, &info.image_count, tmp.data()));

    for (size_t i = 0; i < tmp.size(); i++)
    {
        std::string name = "Swapchain Image " + std::to_string(i);
    }

    /* Swapchain images */
    VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

    for (uint32_t i = 0; i < info.image_count; i++)
    {
        /* Color attachment */
		std::string color_name = "Swapchain Color Image #" + std::to_string(i);
        color_attachments[i].image = tmp[i];
        color_attachments[i].init(colorFormat, info.width, info.height, 1, false, color_name.c_str());
        color_attachments[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)color_attachments[i].image, color_attachments[i].info.debugName);

        /* Depth attachment */
		std::string ds_name = "Swapchain Depth/Stencil Image #" + std::to_string(i);
        depth_attachments[i].init(depthStencilFormat, info.width, info.height, 1, false, ds_name.c_str());
        depth_attachments[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depth_attachments[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
    }

    end_temp_cmd_buffer(cmd_buffer);
}

void VulkanSwapchain::reinitialize()
{
    LOG_INFO("Reinitializing swapchain");
    vkDeviceWaitIdle(context.device);
    destroy();
    init(info.color_format, info.color_space, info.depth_format);
}

void VulkanSwapchain::clear_color(VkCommandBuffer cmd_buffer)
{
    VulkanRenderPassDynamic rp;
    rp.add_color_attachment(color_attachments[context.curr_frame_idx].view);
    rp.begin(cmd_buffer, { info.width, info.height });
    rp.end(cmd_buffer);
}

void VulkanSwapchain::destroy()
{
    for (uint32_t i = 0; i < info.image_count; i++)
    {
        depth_attachments[i].destroy();
    }

    vkDestroySwapchainKHR(context.device, vk_swapchain, nullptr);
}

VkResult VulkanSwapchain::acquire_next_image(VkSemaphore imageAcquiredSmp)
{
    return vkAcquireNextImageKHR(context.device, vk_swapchain, UINT64_MAX, imageAcquiredSmp, VK_NULL_HANDLE, &current_backbuffer_idx);
}

VkResult VulkanSwapchain::present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices)
{
    VkPresentInfoKHR presentInfo =
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount=0,
        .pWaitSemaphores=nullptr,
        .swapchainCount=1,
        .pSwapchains=&vk_swapchain,
        .pImageIndices=&imageIndices,
        //.pResults=,
    };
    
    return vkQueuePresentKHR(queue, &presentInfo);
}

