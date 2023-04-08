#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanModel.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"

static const VkFormat color_attachment_format = VK_FORMAT_R8G8B8A8_SRGB;
/* 
	Number of g-buffers to read from : 
	0: Albedo / 1: View Space Normal / 2: Depth
*/
static constexpr uint32_t num_gbuffers = 3; 

struct DeferredRenderer
{ 
	DeferredRenderer() = default;
	/* 
		@param g_buffers Input attachments from model renderer 
	*/
	void init(std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, std::span<Texture2D> g_buffers_depth);
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
	VkDescriptorSet m_descriptor_set;
	std::array <Texture2D, NUM_FRAMES> m_output_attachment;
	Buffer m_uniform_buffer;


	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_albedo;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_normal;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_depth;
};
