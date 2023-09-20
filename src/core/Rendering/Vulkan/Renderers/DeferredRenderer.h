#pragma once

#include "core/rendering/vulkan/RenderPass.h"
#include "core/rendering/vulkan/VulkanMesh.h"
#include "core/rendering/vulkan/LightManager.h"

struct DeferredRenderer
{ 
	/* 
		@param g_buffers Input attachments from model renderer 
	*/
	void init(std::span<Texture2D> g_buffers_shadow_map, const LightManager& light_manager);
	void render(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer);
	void draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer);
	void update_descriptor_set(size_t frame_idx);
	void reload_shader();
	VkPipeline m_gfx_pipeline;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VertexFragmentShader m_shader_deferred;

	VkDescriptorPool m_descriptor_pool;
	std::array <vk::DynamicRenderPass, NUM_FRAMES> m_lighting_pass;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];

	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_shadow_map;
	LightManager const *h_light_manager;
	Buffer m_material_info_ubo;
};