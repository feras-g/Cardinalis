#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanRenderInterface.h"
#include "VulkanDebugUtils.h"

VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physDevice)
    : hSurface(surface), hPhysicalDevice(physDevice)
{
    const VkInstance& hVkInstance = context.instance;
    const VkDevice& hDevice = context.device;

    // Initialize function pointers
    GET_INSTANCE_PROC_ADDR(hVkInstance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(hVkInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(hVkInstance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(hVkInstance, GetPhysicalDeviceSurfacePresentModesKHR);
    GET_DEVICE_PROC_ADDR(hDevice, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(hDevice, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(hDevice, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(hDevice, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(hDevice, QueuePresentKHR);

    // Present mode
    uint32_t presentModeCount = 0;
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(hPhysicalDevice, hSurface, &presentModeCount, nullptr));
    assert(presentModeCount >= 1);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(hPhysicalDevice, hSurface, &presentModeCount, presentModes.data()));
	info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    LOG_WARN("Current present mode : {0}", string_VkPresentModeKHR(info.presentMode));
}

void VulkanSwapchain::Initialize(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat)
{
    const VkInstance& hVkInstance = context.instance;
    const VkDevice& hDevice = context.device;

    // Query/Set surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(hPhysicalDevice, hSurface, &caps));

    LOG_INFO("Surface capabilities : minImageCount={0} maxImageCount={1}, currentImageExtent={2}x{3}, maxImageExtent={4}x{2}\n",
        caps.minImageCount, caps.maxImageCount, caps.currentExtent.width, caps.currentExtent.height,
        caps.maxImageExtent.width, caps.maxImageExtent.height);

    uint32_t surfaceFormatsCount = 0;
    fpGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, hSurface, &surfaceFormatsCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats (surfaceFormatsCount);
    fpGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, hSurface, &surfaceFormatsCount, surfaceFormats.data());

   
    assert(caps.maxImageCount >= 1);
    info.extent = caps.currentExtent;
    info.imageCount = NUM_FRAMES;// std::min(NUM_FRAMES, caps.maxImageCount);

    info.colorFormat = colorFormat;
    info.colorSpace = colorSpace;
    info.depthStencilFormat = depthStencilFormat;// VK_FORMAT_D32_SFLOAT;

    // Swapchain creation
    VkSwapchainCreateInfoKHR swapchainCreateInfo =
    {
            .sType= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .flags=NULL,
            .surface=hSurface,
            .minImageCount= info.imageCount,
            .imageFormat= info.colorFormat,
            .imageColorSpace= info.colorSpace,
            .imageExtent= info.extent,
            .imageArrayLayers=1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode= VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount=1,
            .pQueueFamilyIndices={0},
            .preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode= info.presentMode,
            .clipped=VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
    };

    VK_CHECK(vkCreateSwapchainKHR(hDevice, &swapchainCreateInfo, nullptr, &swapchain));
    
    depthTextures.resize(info.imageCount);
    color_attachments.resize(info.imageCount);
    framebuffers.resize(info.imageCount);

    std::vector<VkImage> tmp(info.imageCount);
    VK_CHECK(fpGetSwapchainImagesKHR(hDevice, swapchain, &info.imageCount, tmp.data()));

    for (size_t i = 0; i < tmp.size(); i++)
    {
        std::string name = "Swapchain Image " + std::to_string(i);
        set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)tmp[i], name.c_str());
    }

    /* Swapchain images */
    VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

    for (uint32_t i = 0; i < info.imageCount; i++)
    {
        /* Color attachment */
		std::string color_name = "Swapchain Color Image #" + std::to_string(i);
        color_attachments[i].image = tmp[i];
        color_attachments[i].init(colorFormat, info.extent.width, info.extent.height, false);
        color_attachments[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
        color_attachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)color_attachments[i].image, color_name.c_str());

        /* Depth attachment */
		std::string ds_name = "Swapchain Depth/Stencil Image #" + std::to_string(i);

        depthTextures[i].init(depthStencilFormat, info.extent.width, info.extent.height, false);
        depthTextures[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depthTextures[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
    }

    end_temp_cmd_buffer(cmd_buffer);
}

void VulkanSwapchain::Reinitialize()
{
    LOG_INFO("Reinitializing swapchain");
    vkDeviceWaitIdle(context.device);
    Destroy();
    Initialize(info.colorFormat, info.colorSpace, info.depthStencilFormat);
}

void VulkanSwapchain::Destroy()
{
    vkDestroySwapchainKHR(context.device, swapchain, nullptr);
}

VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore imageAcquiredSmp, uint32_t* pBackbufferIndex) const
{
    return fpAcquireNextImageKHR(context.device, swapchain, OneSecondInNanoSeconds, imageAcquiredSmp, VK_NULL_HANDLE, pBackbufferIndex);
}

void VulkanSwapchain::Present(VkCommandBuffer cmdBuffer, VkQueue queue, uint32_t imageIndices)
{
    VkPresentInfoKHR presentInfo =
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount=0,
        .pWaitSemaphores=nullptr,
        .swapchainCount=1,
        .pSwapchains=&swapchain,
        .pImageIndices=&imageIndices,
        //.pResults=,
    };

    VkResult result = fpQueuePresentKHR(queue, &presentInfo);
    assert(result == VK_SUCCESS);
}

inline VkSwapchainKHR VulkanSwapchain::Get() const
{
	return swapchain;
}
