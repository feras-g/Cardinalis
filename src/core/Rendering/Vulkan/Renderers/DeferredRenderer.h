#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include <random>

struct PointLight
{
	glm::vec4 position{};
	glm::vec4 color{};
	glm::vec4 props{};
};

struct LightData
{
	PointLight point_lights[1024];
};

struct LightManager
{
	void init(size_t x, size_t y);
	void init_ubo();
	void update_ubo();

	size_t m_w = 0;
	size_t m_h = 0;

	std::mt19937 rng;

	size_t m_total_size_bytes = 0;
	LightData m_light_data;
	Buffer m_uniform_buffer;

	void destroy() { destroy_buffer(m_uniform_buffer); }
};

static const VkFormat color_attachment_format = VK_FORMAT_R8G8B8A8_SRGB;
/* 
	Number of g-buffers to read from : 
	0: Albedo / 1: View Space Normal / 2: Depth / 3: Normal map / 4: Metallic/Roughness
*/
static constexpr uint32_t num_gbuffers = 5; 

struct DeferredRenderer
{ 
	DeferredRenderer() = default;
	/* 
		@param g_buffers Input attachments from model renderer 
	*/
	void init(
		std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal,
		std::span<Texture2D> g_buffers_depth, std::span<Texture2D> g_buffers_normal_map,
		std::span<Texture2D> g_buffers_metallic_roughness);
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

	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_albedo;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_normal;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_depth;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_normal_map;
	std::array<Texture2D*, NUM_FRAMES> m_g_buffers_metallic_roughness;

	LightManager m_light_manager;

	Buffer m_material_info_ubo;
};