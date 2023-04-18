#pragma once

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/RenderPass.h"

class VulkanShader;

class VulkanModelRenderer 
{
	friend class VulkanImGuiRenderer;
public:
	explicit VulkanModelRenderer();
	void render(size_t currentImageIdx, VkCommandBuffer cmdBuffer);

	void draw_scene(VkCommandBuffer cmdBuffer);
	void update_buffers(const VulkanRendererBase::PerFrameData& frame_data);
	void update_descriptor_set(VkDevice device);
	void create_attachments();
	void create_buffers();

	// Offscreen images
	std::array<Texture2D, NUM_FRAMES> m_Albedo_Output;
	std::array<Texture2D, NUM_FRAMES> m_Normal_Output;
	std::array<Texture2D, NUM_FRAMES> m_Depth_Output;

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];

	/** Size in pixels of the offscreen buffers */
	static const uint32_t render_width  = 2048;
	static const uint32_t render_height = 2048;

	static Buffer m_uniform_buffer;

	~VulkanModelRenderer();
private:
	std::unique_ptr<VulkanShader> m_Shader;

	std::vector<VkFormat> m_ColorAttachmentFormats = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R16G16B16A16_SFLOAT };
	VkFormat m_DepthAttachmentFormat	= VK_FORMAT_D16_UNORM;

	VkSampler m_TextureSampler;
	Texture2D m_default_texture;

	VkPipeline			m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_ppl_layout;
	
	VkDescriptorSetLayout m_pass_descriptor_set_layout;
	VkDescriptorSet m_pass_descriptor_set;
};
