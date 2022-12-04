#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <vulkan/vulkan.h>


struct ImageInfo
{
	unsigned int width;
	unsigned int height;
	VkFormat format;
	uint32_t layerCount = 1;
	VkImageUsageFlags usage		  = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t mipLevels = 1;
	VkMemoryPropertyFlags memoryProperties = 0;

	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageCreateFlagBits flags;
};

struct ImageViewInfo
{
	VkFormat format;
	VkImageAspectFlags aspectFlags;
	uint32_t mipLevels = 1;
};

struct VulkanTexture
{
	VkImage image			= VK_NULL_HANDLE;
	VkImageView view		= VK_NULL_HANDLE;
	VkDeviceMemory memory	= VK_NULL_HANDLE;

	ImageInfo info;
	ImageViewInfo viewInfo;

	bool CreateImage(VkDevice device, const ImageInfo& info);
	bool CreateImageFromData(VkCommandBuffer cmdBuffer, VkDevice device, const ImageInfo& info, void* data);
	bool UpdateImageTextureData(VkCommandBuffer cmdBuffer, VkDevice device, void* data);
	bool CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer);
	bool CreateImageView(VkDevice device, const ImageViewInfo& info);

	// Records an image transition command for a single image.
	void Transition(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	// Reset image info and destroy image and associated view
	void Destroy(VkDevice device);
};



#endif // !VULKAN_TEXTURE_H
