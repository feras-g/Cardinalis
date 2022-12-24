#include "VulkanTexture.h"

#include "Rendering/Vulkan/VulkanTools.h"

static VkAccessFlags GetAccessMask(VkImageLayout layout);
static uint32_t GetBytesPerPixelFromFormat(VkFormat format);
static void GetSrcDstPipelineStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags& out_srcStageMask, VkPipelineStageFlags& out_dstStageMask);

bool VulkanTexture::CreateFromFile(const char* filename, VkFormat format, VkImageAspectFlags imageAspect)
{
    info.format = format;
    info.aspectFlag = imageAspect;
    Image texture = LoadImageFromFile(filename, info);
    return CreateImageFromData(context.device, info, texture.get());
}

bool VulkanTexture::CreateImage(VkDevice device, const ImageInfo& info)
{
    this->info = info;

    bool isCube = (info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImageCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = info.format,
        .extent = { info.width, info.height, 1 },
        .mipLevels = info.mipLevels,
        .arrayLayers = (uint32_t)(isCube ? 6 : 1),
        .samples = VK_SAMPLE_COUNT_1_BIT, // No multisampling
        .tiling = info.tiling, // VK_IMAGE_TILING_OPTIMAL
        .usage = info.usage, //VK_IMAGE_USAGE_SAMPLED_BIT
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices= nullptr,
        .initialLayout= VK_IMAGE_LAYOUT_UNDEFINED,
    };


    VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &image));

    // Image memory
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

bool VulkanTexture::CreateImageFromData(VkDevice device, const ImageInfo& info, void* data)
{
    CreateImage(device, info);
    UpdateImageTextureData(device, data);

    return true;
}

bool VulkanTexture::UpdateImageTextureData(VkDevice device, void* data)
{
    uint32_t bpp = GetBytesPerPixelFromFormat(info.format);

    VkDeviceSize layerSizeInBytes = info.width * info.height * bpp;
    VkDeviceSize imageSizeInBytes = layerSizeInBytes * info.layerCount;
    
    // Create temp CPU-GPU visible buffer holding image data 
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMem;
    CreateBuffer(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMem);
    UploadBufferData(stagingBufferMem, 0, data, imageSizeInBytes);

    // Copy content to image memory on GPU
    StartInstantUseCmdBuffer();
    Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(context.mainCmdBuffer, stagingBuffer);
    Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EndInstantUseCmdBuffer();

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMem, nullptr);

    return true;
}

bool VulkanTexture::CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer)
{
    VkBufferImageCopy imageRegion =
    {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers {
            .aspectMask = info.aspectFlag,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = info.layerCount
        },
        .imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
        .imageExtent = { info.width, info.height, 1 }
    };

    vkCmdCopyBufferToImage(cmdBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

    return true;
}

bool VulkanTexture::CreateImageView(VkDevice device, const ImageViewInfo& info)
{
    this->viewInfo = info;

    VkImageViewCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        //.flags = ,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = info.format,
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

    VkResult result = vkCreateImageView(context.device, &createInfo, nullptr, &view);
    assert(result == VK_SUCCESS);

    return true;
}

void VulkanTexture::Destroy(VkDevice device)
{
    info = {};

    vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(context.device, memory, nullptr);
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
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = viewInfo.mipLevels, // Transition all mip levels by default
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || 
            (info.format == VK_FORMAT_D16_UNORM)         || (info.format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
            (info.format == VK_FORMAT_D32_SFLOAT)        || (info.format == VK_FORMAT_S8_UINT) ||
            (info.format == VK_FORMAT_D16_UNORM_S8_UINT) || (info.format == VK_FORMAT_D24_UNORM_S8_UINT))
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (info.format == VK_FORMAT_D32_SFLOAT_S8_UINT || info.format == VK_FORMAT_D24_UNORM_S8_UINT)
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        VkPipelineStageFlags srcStageMask = 0;
        VkPipelineStageFlags dstStageMask = 0;
        GetSrcDstPipelineStage(info.layout, newLayout, srcStageMask, dstStageMask);

        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, NULL, 0, NULL, 1, &barrier);

        info.layout = newLayout;
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

uint32_t GetBytesPerPixelFromFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R16_SFLOAT:
        return 2;
    case VK_FORMAT_R16G16_SFLOAT:
        return 4;
    case VK_FORMAT_R16G16_SNORM:
        return 4;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return 4;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 4 * sizeof(uint16_t);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * sizeof(float);
    default:
        break;
    }
    return 0;
}

void GetSrcDstPipelineStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags& out_srcStageMask, VkPipelineStageFlags& out_dstStageMask)
{
    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
    }

    else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
    }

    else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // X -> Shader read-only
    {
        if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        // Depth-Stencil Attachment -> Shader Read-only
        if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        // Color Attachment -> Shader Read-only
        if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            out_srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            out_dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
    }
}
