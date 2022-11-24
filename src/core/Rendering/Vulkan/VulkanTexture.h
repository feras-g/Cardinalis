#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <vulkan/vulkan.h>


struct TextureInfo
{
	VkFormat	format;
	VkExtent3D	dimensions;
	VkImageUsageFlags usage;
	uint32_t mipLevels;
	VkMemoryPropertyFlags memoryProperties;

	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

struct TextureViewInfo
{
	VkFormat viewFormat;
	VkImageAspectFlags aspectFlags;
	uint32_t mipLevels = 1;
};

struct VulkanTexture
{
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;

	TextureInfo info;
	TextureViewInfo viewInfo;

	bool CreateTexture2D(VkDevice device, const TextureInfo& info);
	bool CreateTexture2DView(VkDevice device, const TextureViewInfo& info);

	// Records an image transition command for a single image.
	void Transition(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	// Reset image info and destroy image and associated view
	void Destroy(VkDevice device);
};



#endif // !VULKAN_TEXTURE_H
