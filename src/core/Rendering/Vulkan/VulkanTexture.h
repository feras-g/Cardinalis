#pragma once

#include <vulkan/vulkan.hpp>
#include <string_view>

struct Image;

struct TextureInfo
{
	VkFormat imageFormat		{ VK_FORMAT_UNDEFINED };
	uint32_t width				{ 0 };
	uint32_t height				{ 0 };
	uint32_t mipLevels			{ 1 };
	uint32_t layerCount			{ 1 };
	VkImageLayout imageLayout	{ VK_IMAGE_LAYOUT_UNDEFINED };
	const char* debugName		{ nullptr };
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

static constexpr ImageViewInitInfo ImageViewCubeTexture
{
	VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6
};

struct Texture
{
	VkImage               image			{ VK_NULL_HANDLE };
	VkImageView		      view			{ VK_NULL_HANDLE };
	VkDeviceMemory        deviceMemory	{ VK_NULL_HANDLE };
	VkSampler             sampler		{ VK_NULL_HANDLE };
	VkDescriptorImageInfo descriptor	{ VK_NULL_HANDLE };
	bool initialized					{ false };		/* True if TextureInfo has been initialized through init(). */
	TextureInfo			  info;

	void create_view(VkDevice device, const ImageViewInitInfo& info);
	void copy_from_buffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer);
	void upload_data(VkDevice device, Image* pImage);
	void transition_layout(VkCommandBuffer cmdBuffer, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange* subresourceRange = nullptr);
	void destroy(VkDevice device);
	void generate_mipmaps();

	/* Create and allocated memory for a Vulkan image */
	void create_vk_image(VkDevice device, bool isCubemap, VkImageUsageFlags imageUsage);
	void create_vk_image_cube(VkDevice device, VkImageUsageFlags imageUsage);
};



struct Texture2D : public Texture
{
public:
	void init(VkFormat format, uint32_t width, uint32_t height, uint32_t layers, bool calc_mip);

	void create_from_file(
		std::string_view	filename,
		std::string_view    debug_name,
		VkFormat			format,
		VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout		layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void create_from_data(
		Image*				pImage,
		std::string_view    debug_name,
		VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout		layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void create(VkDevice device, VkImageUsageFlags imageUsage, std::string_view debug_name = "");

	/* Temporary */
	static VkImageView create_texture_cube_view(Texture2D texture);
	static VkImageView create_texture_array_view(Texture2D texture);
};

