#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/LightManager.h"

struct DeferredRenderer
{ 
	DeferredRenderer() = default;
	/* 
		@param g_buffers Input attachments from model renderer 
	*/
	void init(
		std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal,
		std::span<Texture2D> g_buffers_depth, std::span<Texture2D> g_buffers_normal_map,
		std::span<Texture2D> g_buffers_metallic_roughness, std::span<Texture2D> g_buffers_shadow_map, const LightManager& light_manager);
	void render(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer);
	void draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer);
	void create_uniform_buffers();
	void update(size_t frame_idx);
	void update_descriptor_set(size_t frame_idx);
	VkPipeline m_gfx_pipeline;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VulkanShader m_shader_deferred;

	VkDescriptorPool m_descriptor_pool;
	std::array <vk::DynamicRenderPass, NUM_FRAMES> m_dyn_renderpass;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	std::array <Texture2D, NUM_FRAMES> m_output_attachment;

	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_albedo;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_normal;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_depth;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_normal_map;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_metallic_roughness;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_shadow_map;
	LightManager const *h_light_manager;
	Buffer m_material_info_ubo;
};