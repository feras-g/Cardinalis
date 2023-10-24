#include "VulkanTexture.h"

#include "core/rendering/vulkan/VulkanTools.h"
#include "core/rendering/vulkan/VulkanDebugUtils.h"
#include "core/rendering/vulkan/VkResourceManager.h"

static VkAccessFlags get_src_access_mask(VkImageLayout layout);

static uint32_t GetBytesPerPixelFromFormat(VkFormat format);
static void GetSrcDstPipelineStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags& out_srcStageMask, VkPipelineStageFlags& out_dstStageMask);

static uint32_t calc_mip_levels(uint32_t width, uint32_t height)
{
    return uint32_t(std::floor(std::log2(std::max(width, height))) + 1);
}

void Texture2D::init(VkFormat format, uint32_t width, uint32_t height, uint32_t layers, bool calc_mip, std::string_view debug_name)
{
    info.imageFormat = format;
	info.width  = width;
	info.height = height;
	info.mipLevels = calc_mip ? calc_mip_levels(width, height) : 1;
    info.layerCount = layers;
	info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.debugName = debug_name.data();

    initialized = true;
}

void Texture2D::create_from_file(
    std::string_view	filename,
    VkFormat			format,
    VkImageUsageFlags	imageUsage,
    VkImageLayout		layout)
{
    assert(initialized);
    Image rawImage = load_image_from_file(filename);
    create_from_data(&rawImage, imageUsage, layout);
}

void Texture2D::create_from_data(
    void*               data,
    VkImageUsageFlags	imageUsage,
    VkImageLayout		layout)
{
	assert(initialized);
    create_vk_image(context.device, false, imageUsage);
    
    transition_immediate(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);

    upload_data(context.device, data);

	transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

    if (info.mipLevels > 1)
    {
        generate_mipmaps();
    }

}

void Texture2D::create_from_data(
	Image*               image,
	VkImageUsageFlags	imageUsage,
	VkImageLayout		layout)
{
	create_from_data(image->get_data(), imageUsage, layout);
}

void Texture2D::create(VkDevice device, VkImageUsageFlags imageUsage)
{
    create_vk_image(device, false, imageUsage);
}
void Texture::create_vk_image_cube(VkDevice device, VkImageUsageFlags imageUsage)
{
    create_vk_image(device, true, imageUsage);
}
void Texture::create_vk_image(VkDevice device, bool isCubemap, VkImageUsageFlags imageUsage)
{
    assert(info.debugName);
    VkImageCreateFlags flags{ 0 };
    uint32_t arrayLayers = 1u;

    VkImageCreateInfo createInfo =
    {
        .sType                  { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
		.flags                  { VK_IMAGE_CREATE_EXTENDED_USAGE_BIT },
        .imageType              { VK_IMAGE_TYPE_2D },
        .format                 { info.imageFormat },
        .extent                 { info.width, info.height, 1 },
        .mipLevels              { info.mipLevels },
        .arrayLayers            { info.layerCount },
        .samples                { VK_SAMPLE_COUNT_1_BIT },
        .tiling                 { VK_IMAGE_TILING_OPTIMAL },
        .usage                  { imageUsage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT },
        .sharingMode            { VK_SHARING_MODE_EXCLUSIVE },
        .queueFamilyIndexCount  { 0 },
        .pQueueFamilyIndices    { nullptr },
        .initialLayout          { VK_IMAGE_LAYOUT_UNDEFINED }
    };

    if (isCubemap)
    {
        createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &image));

    // Image memory
    VkMemoryRequirements imageMemReq;
    vkGetImageMemoryRequirements(device, image, &imageMemReq);
    
    VkMemoryAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemReq.size,
        .memoryTypeIndex = EngineUtils::FindMemoryType(context.physical_device, imageMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };
    
    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &deviceMemory));
    VK_CHECK(vkBindImageMemory(device, image, deviceMemory, 0));

    /* Add to resource manager */
    hash = VkResourceManager::get_instance(device)->add_image(image, deviceMemory);

    set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)image, info.debugName);
}

void Texture::copy_from_buffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer)
{
	VkBufferImageCopy imageRegion =
	{
		.bufferOffset       { 0 },
		.bufferRowLength    { 0 },
		.bufferImageHeight  { 0 },
		.imageSubresource
		{
			.aspectMask     { VK_IMAGE_ASPECT_COLOR_BIT },
			.mipLevel       { 0 },
			.baseArrayLayer { 0 },
			.layerCount     { info.layerCount }
		},
		.imageOffset {.x = 0, .y = 0, .z = 0 },
		.imageExtent { info.width, info.height, 1 }
	};

	vkCmdCopyBufferToImage(
		cmdBuffer,
		srcBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
}

void Texture::upload_data(VkDevice device, void* data)
{
    uint32_t bpp = GetBytesPerPixelFromFormat(info.imageFormat);
    VkDeviceSize layerSizeInBytes = info.width * info.height * bpp;
    VkDeviceSize imageSizeInBytes = layerSizeInBytes * info.layerCount;
    // Create temp CPU-GPU visible buffer holding image data 
    Buffer stagingBuffer;
	stagingBuffer.init(Buffer::Type::STAGING, imageSizeInBytes, "Image Staging Buffer");
    stagingBuffer.create();
	stagingBuffer.upload(context.device, data, 0, imageSizeInBytes);

    VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
    copy_from_buffer(cmd_buffer, stagingBuffer);
    end_temp_cmd_buffer(cmd_buffer);
}

void Texture::create_view(VkDevice device, const ImageViewInitInfo& viewInfo)
{
	view = create_texture_view(*this, info.imageFormat, viewInfo.viewType, viewInfo.aspectMask);
}

void Texture::destroy()
{
    if (hash)
    {
        VkResourceManager::get_instance(context.device)->destroy_image(hash);
    }

    if (view_hash)
    {
        VkResourceManager::get_instance(context.device)->destroy_image_view(view_hash);
    }

    *this = {};
}

void Texture::generate_mipmaps()
{




    //   VkCommandBuffer cbuf = begin_temp_cmd_buffer();

	///* TODO : Compute shader */

    //   end_temp_cmd_buffer(cbuf);

    VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

    int32_t mip_height = info.height;
    int32_t mip_width = info.width;
    for (uint32_t mip_level = 1; mip_level < info.mipLevels; mip_level++)
    {
        /* Transition source mip-level */
        uint32_t src_mip_level = mip_level - 1;
        uint32_t dst_mip_level = mip_level;
        VkImageSubresourceRange src_subresouce_range =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = mip_level - 1,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        transition(cmd_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, &src_subresouce_range);

        /* Blit from mip level N to N-1 */
        VkImageBlit blit =
        {
            /* source */
            .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = src_mip_level,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets =   /* bounds of the source region */
            {
                { 0, 0, 0 },
                { mip_width, mip_height, 1 }
            },
            /* destination */
            .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = dst_mip_level,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = /* bounds of the destination region */
            {
                { 0, 0, 0 },
                { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 }
            }
        };
        vkCmdBlitImage(cmd_buffer,
            this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  /* source */
            this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  /* destination */
            1, &blit, VK_FILTER_LINEAR);

        /* Transition source to final state */
        transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT , &src_subresouce_range);

        /* Prepare for next blit. */
        mip_width = mip_width > 1 ? mip_width / 2 : 1;
        mip_height = mip_height > 1 ? mip_height / 2 : 1;
    }

    /* Transition last mip level */
    VkImageSubresourceRange subresouce_range =
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = info.mipLevels - 1,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, &subresouce_range);

    end_temp_cmd_buffer(cmd_buffer);
}

// Helper to transition images and remember the current layout
void Texture::transition(VkCommandBuffer cmdBuffer, VkImageLayout new_layout, VkAccessFlags dst_access_mask, VkImageSubresourceRange* subresourceRange)
{
	//if (new_layout == info.imageLayout)
	//	return;
    
    VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = get_src_access_mask(info.imageLayout),
		.dstAccessMask = dst_access_mask,
        .oldLayout = info.imageLayout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange ? *subresourceRange :
        VkImageSubresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = info.mipLevels, // Transition all mip levels by default
            .baseArrayLayer = 0,
            .layerCount = info.layerCount,
        }
    };

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || 
        (info.imageFormat == VK_FORMAT_D16_UNORM)         || (info.imageFormat == VK_FORMAT_X8_D24_UNORM_PACK32) ||
        (info.imageFormat == VK_FORMAT_D32_SFLOAT)        || (info.imageFormat == VK_FORMAT_S8_UINT) ||
        (info.imageFormat == VK_FORMAT_D16_UNORM_S8_UINT) || (info.imageFormat == VK_FORMAT_D24_UNORM_S8_UINT) || (info.imageFormat == VK_FORMAT_D32_SFLOAT_S8_UINT))
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (info.imageFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || info.imageFormat == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    VkPipelineStageFlags srcStageMask = 0;
    VkPipelineStageFlags dstStageMask = 0;
	GetSrcDstPipelineStage(info.imageLayout, new_layout, srcStageMask, dstStageMask);

    vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, NULL, 0, NULL, 1u, &barrier);

    info.imageLayout = new_layout;

}

/* The image layout transition is executed immediately */
void Texture::transition_immediate(VkImageLayout new_layout, VkAccessFlags dst_access_mask, VkImageSubresourceRange* subresourceRange)
{
	VkCommandBuffer cmd_buff = begin_temp_cmd_buffer();
	transition(cmd_buff, new_layout, dst_access_mask, subresourceRange);
	end_temp_cmd_buffer(cmd_buff);
}

/* https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageMemoryBarrier.html */

VkAccessFlags get_src_access_mask(VkImageLayout layout)
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
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_ACCESS_NONE;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_NONE;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_ACCESS_NONE;
    default:
        assert(false);
        break;
    }

    return VK_ACCESS_NONE;
}

uint32_t GetBytesPerPixelFromFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8_SINT:
        return 1;
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R8G8_UNORM:
        return 2;
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
    case VK_FORMAT_R8G8B8A8_SRGB:
        return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;
    case VK_FORMAT_R16G16B16A16_UNORM:
        return 8;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    default:
        assert(false);
        return 0;
        break;
    }
}

void GetSrcDstPipelineStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags& out_srcStageMask, VkPipelineStageFlags& out_dstStageMask)
{
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAccessFlagBits.html

    /* Src Stage */
    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        out_srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        out_srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        out_srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		out_srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		out_srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		out_srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        out_srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
		out_srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
    case VK_IMAGE_LAYOUT_GENERAL:
        out_srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        break;
    default:
        LOG_ERROR("Unsupported old_layout.");
        assert(false);
    }

    /* Dst Stage */
    switch (newLayout)
    {
    //case VK_IMAGE_LAYOUT_UNDEFINED:
    //    break;
    //case VK_IMAGE_LAYOUT_GENERAL:
    //    break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		out_dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		out_dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    //case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    //    break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		out_dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        out_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        out_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    //case VK_IMAGE_LAYOUT_PREINITIALIZED:
    //    break;
    //case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    //    break;
    //case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    //    break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
		out_dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    //case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    //    break;
    //case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    //    break;
    //case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    //    break;
    case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
        out_dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    //case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
    //    break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        out_dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
    case VK_IMAGE_LAYOUT_GENERAL:
        out_dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        break;
    //case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
    //    break;
    //case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR:
    //    break;
    //case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR:
    //    break;
    //case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
    //    break;
    //case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
    //    break;
    //case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
    //    break;
    //case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
    //    break;
    //case VK_IMAGE_LAYOUT_MAX_ENUM:
    //    break;
    default:
		LOG_ERROR("Unsupported new_layout.");
		assert(false);
        break;
    }
}

VkImageView create_texture_view(
	Texture& texture, VkFormat format, VkImageViewType view_type,
	VkImageAspectFlags aspect, VkImageSubresourceRange* subresourceRange)
{
	VkImageViewCreateInfo createInfo =
	{
		.sType      { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
		.flags      {  },
		.image      { texture.image },
		.viewType   { view_type },
		.format     { format },
		.components
		{
			.r = VK_COMPONENT_SWIZZLE_R,
			.g = VK_COMPONENT_SWIZZLE_G,
			.b = VK_COMPONENT_SWIZZLE_B,
			.a = VK_COMPONENT_SWIZZLE_A
		},
	};

	if (subresourceRange != nullptr)
	{
		createInfo.subresourceRange = *subresourceRange;
	}
	else
	{
		createInfo.subresourceRange =
		{
			.aspectMask     { aspect },
			.baseMipLevel   { 0 },
			.levelCount     { texture.info.mipLevels },
			.baseArrayLayer { 0 },
			.layerCount     { texture.info.layerCount },
		};
	}

	VkImageView out_view;
	vkCreateImageView(context.device, &createInfo, nullptr, &out_view);

    /* Add to resource manager */
    texture.view_hash = VkResourceManager::get_instance(context.device)->add_image_view(out_view);

	return out_view;
}

VkImageView Texture2D::create_texture_2d_view(Texture2D texture, VkFormat format, 
                                              VkImageAspectFlags aspect)
{
	return create_texture_view(texture, format, 
	                    VK_IMAGE_VIEW_TYPE_2D, aspect);
}

VkImageView Texture2D::create_texture_cube_view(Texture2D texture, VkFormat format, 
                                                VkImageAspectFlags aspect)
{
	return create_texture_view(texture, format, 
	                    VK_IMAGE_VIEW_TYPE_CUBE, aspect);
}

VkImageView Texture2D::create_texture_2d_array_view(Texture2D texture, VkFormat format, 
                                                    VkImageAspectFlags aspect)
{
	return create_texture_view(texture, format, 
	                    VK_IMAGE_VIEW_TYPE_2D_ARRAY, aspect);
}
