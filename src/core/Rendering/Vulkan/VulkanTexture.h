#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <vulkan/vulkan.h>

VkImage		CreateTexture2D(VkDevice device, VkExtent3D extent3D, VkFormat format, uint32_t mipLevels);
VkImageView	CreateTexture2DView(VkDevice device, VkImage image, VkFormat viewFormat, uint32_t mipLevels);

struct TextureMetadata
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageType type = VK_IMAGE_TYPE_2D;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	uint32_t mipLevels = 1;
};

struct VulkanTexture
{
	VkImage image;
	VkImageView view;
	TextureMetadata info;

	// Records an image transition command for a single image.
	void Transition(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	// Reset image info and destroy image and associated view
	void Destroy(VkDevice device);
};



#endif // !VULKAN_TEXTURE_H
