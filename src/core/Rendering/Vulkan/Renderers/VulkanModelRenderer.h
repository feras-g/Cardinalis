#pragma once

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/LightManager.h"

class VulkanShader;

class VulkanModelRenderer 
{
	friend class VulkanImGuiRenderer;
public:
	explicit VulkanModelRenderer();
	void render(size_t currentImageIdx, VkCommandBuffer cmd_buffer);

	void draw_scene(VkCommandBuffer cmdBuffer, size_t current_frame_idx, const Drawable& drawable);
	void update(size_t frame_idx, const VulkanRendererBase::PerFrameData& frame_data);
	void update_descriptor_set(VkDevice device, size_t frame_idx);
	void create_attachments();
	void create_buffers();

	/* G-Buffers for Deferred rendering */
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_albdedo;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_normal;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_depth;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_directional_shadow;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_metallic_roughness;

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];

	static uint32_t render_width;
	static uint32_t render_height;

	~VulkanModelRenderer();
private:
	std::unique_ptr<VulkanShader> m_shader;

	std::vector<VkFormat> m_formats = 
	{ 
		tex_base_color_format,				/* Base color / Albedo */
		VK_FORMAT_R16G16B16A16_SFLOAT,		/* Vertex normal */
		tex_metallic_roughness_format,		/* Metallic roughness */
	};
	VkFormat m_depth_format	= VK_FORMAT_D16_UNORM;

	VkSampler m_TextureSampler;

	VkPipeline			m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_ppl_layout;
	
	VkDescriptorSetLayout m_pass_descriptor_set_layout;
	VkDescriptorSet m_pass_descriptor_set;

	Buffer m_material_info_ubo;
};
