#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/DescriptorSet.h"


struct Texture2D;
struct VkCommandBuffer_T;

enum PostFX
{
	eDownsample,
	eBloom,
	eCircularDof,
	eSSAO,
	eCount
};

struct PostFX_Inputs
{
	Texture2D* color = nullptr;
	Texture2D* depth = nullptr;
};

struct PostFX_Downsample
{
	void init();
	void render(VkCommandBuffer_T* cmd_buff);
	DescriptorSet desc_set = VK_NULL_HANDLE;
	VkPipeline compute_ppl = VK_NULL_HANDLE;
	ComputeShader m_downsample_cs;
	Texture2D output_image;
	VkPipelineLayout ppl_layout;
	uint32_t width;
	uint32_t height;
	Texture2D* input_image_handle = nullptr;
};

struct PostProcessRenderer
{
	void init();
	void render(PostFX fx, VkCommandBuffer_T* cmd_buff, const Texture2D& input_image);

	PostFX_Downsample m_postfx_downsample;
};