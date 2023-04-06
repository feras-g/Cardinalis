#pragma once

#include <vulkan/vulkan.h>
#include <string_view>

struct TextureInfo
{
	VkFormat imageFormat		{ VK_FORMAT_UNDEFINED };
	uint32_t width				{ 0 };
	uint32_t height				{ 0 };
	uint32_t mipLevels			{ 1 };
	uint32_t layerCount			{ 1 };
	VkImageLayout imageLayout	{ VK_IMAGE_LAYOUT_UNDEFINED };
	const char* debugName;
};

struct ImageViewInitInfo
{
	VkImageViewType    viewType			{ VK_IMAGE_VIEW_TYPE_2D };
	VkImageAspectFlags aspectMask		{ VK_IMAGE_ASPECT_COLOR_BIT };
	uint32_t		   baseMipLevel		{ 0 };
	uint32_t		   levelCount		{ 1 };
	uint32_t		   baseArrayLayer	{ 0 };
	uint32_t		   layerCount		{ 1 };
};

struct Texture
{
	VkImage               image			{ nullptr };
	VkImageView		      view			{ nullptr };
	VkDeviceMemory        deviceMemory	{ nullptr };
	VkSampler             sampler		{ nullptr };
	VkDescriptorImageInfo descriptor	{ nullptr };
	TextureInfo			  info;

	void CreateView(VkDevice device, const ImageViewInitInfo& info);
	void CopyFromBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer);
	void UpdateData(VkDevice device, void const* const data);
	void transition_layout(VkCommandBuffer cmdBuffer, VkImageLayout old_layout, VkImageLayout new_layout);
	void Destroy(VkDevice device);
};

/* Create and allocated memory for a Vulkan image */
void CreateImage(VkDevice device, bool isCubemap, Texture& texture, VkImageUsageFlags imageUsage);

struct Texture2D : public Texture
{
public:
	void CreateFromFile(
		std::string_view	filename,
		std::string_view    debug_name,
		VkFormat			format,
		VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout		layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void CreateFromData(
		void const* const   data,
		std::string_view    debug_name,
		size_t				sizeInBytes,
		TextureInfo			info,
		VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout		layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void CreateImage(VkDevice device, VkImageUsageFlags imageUsage, std::string_view debug_name = "");
};
