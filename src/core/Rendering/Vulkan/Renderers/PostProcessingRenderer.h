#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/DescriptorSet.h"


struct Texture2D;
struct VkCommandBuffer_T;

enum PostFX
{
	eDownsample,
	eSSR,
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

struct PostFX_Base
{
	DescriptorSet descriptor_set = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	ComputeShader shader;
};

struct PostFX_Downsample : PostFX_Base
{
	void init();
	void render(VkCommandBuffer_T* cmd_buff);
	Texture2D* input_image_handle = nullptr;
	Texture2D output_image;
	uint32_t width;
	uint32_t height;
};

struct PostFX_SSR : PostFX_Base
{
	void init();
	void render(VkCommandBuffer_T* cmd_buff);
};

struct PostFX_Blur : PostFX_Base
{
	void init();
	void render(VkCommandBuffer_T* cmd_buff);
};

struct PostProcessRenderer : PostFX_Base
{
	void init();
	void render(PostFX fx, VkCommandBuffer_T* cmd_buff, const Texture2D& input_image);

	PostFX_Downsample m_postfx_downsample;
	PostFX_Blur m_postfx_blur;
};