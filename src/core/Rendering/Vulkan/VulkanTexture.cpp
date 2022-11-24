#include "VulkanTexture.h"

#include "Rendering/Vulkan/VulkanTools.h"

static VkAccessFlags GetAccessMask(VkImageLayout layout);

// Returns a handle to the newly create image
bool VulkanTexture::CreateTexture2D(VkDevice device, const TextureInfo& info)
{
    this->info = info;

    VkImageCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = info.format,
        .extent = info.dimensions,
        .mipLevels = info.mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT, // No multisampling
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = info.usage, //VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices= nullptr,
        .initialLayout= VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkResult result = vkCreateImage(device, &createInfo, nullptr, &image);
    assert(result == VK_SUCCESS);


    VkMemoryRequirements imageMemReq;
    vkGetImageMemoryRequirements(device, image, &imageMemReq);

    VkMemoryAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemReq.size,
        .memoryTypeIndex = FindMemoryType(context.physicalDevice, imageMemReq.memoryTypeBits, info.memoryProperties)
    };
    
    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
    VK_CHECK(vkBindImageMemory(device, image, memory, 0));

    return true;
}

bool VulkanTexture::CreateTexture2DView(VkDevice device, const TextureViewInfo& info)
{
    this->viewInfo = info;

    VkImageViewCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        //.flags = ,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = info.viewFormat,
        .components = 
        {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        },
        .subresourceRange =
        {
            .aspectMask     = info.aspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = info.mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    VkResult result = vkCreateImageView(device, &createInfo, nullptr, &view);
    assert(result == VK_SUCCESS);

    return true;
}

void VulkanTexture::Destroy(VkDevice device)
{
    info = {};

    vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
}

// Helper to transition images and remember the current layout
void VulkanTexture::Transition(VkCommandBuffer cmdBuffer, VkImageLayout newLayout)
{
    if (info.layout != newLayout)
    {
        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = GetAccessMask(info.layout),
            .dstAccessMask = GetAccessMask(newLayout),
            .oldLayout = info.layout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange =
            {
                .aspectMask = viewInfo.aspectFlags,
                .baseMipLevel = 0,
                .levelCount = viewInfo.mipLevels, // Transition all mip levels by default
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        info.layout = newLayout;

        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
}

VkAccessFlags GetAccessMask(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
    default:
        break;
    }

    return VK_ACCESS_NONE;
}

