#include "VulkanSwapchain.h"

#include <algorithm>

#include "VulkanRenderInterface.h"

VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physDevice)
    : hSurface(surface), hPhysicalDevice(physDevice)
{
}

void VulkanSwapchain::Initialize(VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkFormat depthStencilFormat)
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

    // Query/Set surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(hPhysicalDevice, hSurface, &caps));

    LOG_INFO("Surface capabilities : maxImageCount={0}, currentImageExtent={1}x{2}, maxImageExtent={3}x{1}\n", 
            caps.maxImageCount, caps.currentExtent.width, caps.currentExtent.height, 
            caps.maxImageExtent.width, caps.maxImageExtent.height);

    assert(caps.maxImageCount >= 1);
    info.extent = caps.currentExtent;

    // Present mode
    uint32_t presentModeCount = 0;
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(hPhysicalDevice, hSurface, &presentModeCount, nullptr));
    assert(presentModeCount >= 1);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(hPhysicalDevice, hSurface, &presentModeCount, presentModes.data()));

    info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // This is required
    info.imageCount  = min(NUM_FRAMES, caps.maxImageCount);
    info.colorFormat = colorFormat;
    info.colorSpace = colorSpace;
    info.depthStencilFormat = depthStencilFormat;// VK_FORMAT_D32_SFLOAT;

    // Depth-Stencil 
    VkExtent3D depthExtent = { info.extent.width, info.extent.height, 1 };

    // Create swapchain
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
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
    LOG_INFO("vkCreateSwapchainKHR() success.");
    
    depthImages.resize(info.imageCount);
    images.resize(info.imageCount);
    framebuffers.resize(info.imageCount);
    std::vector<VkImage> tmp(info.imageCount);
    VK_CHECK(fpGetSwapchainImagesKHR(hDevice, swapchain, &info.imageCount, tmp.data()));
    LOG_INFO("GetSwapchainImagesKHR() success.");

    // Swapchain images creation
    BeginRecording(context.mainCmdBuffer);
    for (int i = 0; i < info.imageCount; i++)
    {
        // COLOR
        images[i].image = tmp[i];
        images[i].info =
        {
            .format = colorFormat,
            .dimensions = { info.extent.width, info.extent.height, 4 },
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .mipLevels = 1,
            .memoryProperties = 0,
        };
        images[i].CreateTexture2DView(context.device, 
        {
            .format      = colorFormat,
            .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevels   = 1
        });
        images[i].Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // DEPTH
        depthImages[i].CreateTexture2D(context.device,
        {
            .format     = depthStencilFormat,
            .dimensions = depthExtent,
            .usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .mipLevels  = 1,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        });
        depthImages[i].CreateTexture2DView(context.device, { .format = depthStencilFormat, .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT, .mipLevels = 1 });
        depthImages[i].Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    EndRecording(context.mainCmdBuffer);
}

void VulkanSwapchain::Reinitialize(VkFormat format, VkColorSpaceKHR colorSpace)
{
}

void VulkanSwapchain::Destroy()
{
    info = {};

    for (int i = 0; i < info.imageCount; i++)
    {
        images[i].Destroy(context.device);
        vkDestroyFramebuffer(context.device, framebuffers[i], nullptr);
    }

    vkDestroySwapchainKHR(context.device, swapchain, nullptr);
}

void VulkanSwapchain::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* pBackbufferIndex) const
{
    VK_CHECK(fpAcquireNextImageKHR(context.device, swapchain, OneSecondInNanoSeconds, presentCompleteSemaphore, VK_NULL_HANDLE, pBackbufferIndex));
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
