#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include <span>

struct Drawable;
struct LightManager;
struct Texture2D;
class Camera;

struct ShadowRenderer
{ 
	void init(unsigned int width, unsigned int height, const LightManager& lightmanager);
	void render(size_t current_frame_idx, VkCommandBuffer cmd_buffer);
	
	void draw_scene(VkCommandBuffer cmd_buffer);
	void update_desc_sets();
	glm::uvec2 m_shadow_map_size;

	Texture2D m_shadow_maps[NUM_FRAMES];

	VulkanShader m_shadow_shader;
	vk::DynamicRenderPass m_shadow_pass[NUM_FRAMES];
	
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_gfx_pipeline_layout;
	VkPipeline m_gfx_pipeline;
	const LightManager* h_light_manager;
};

constexpr int NUM_CASCADES = 4;
static const uint32_t view_mask = 0b00001111;

struct CascadedShadowRenderer
{
	void init(unsigned int width, unsigned int height, const Camera& camera, const LightManager& lightmanager);
	void update_desc_sets();

	Texture2D m_shadow_maps[NUM_FRAMES];
	vk::DynamicRenderPass m_CSM_pass[NUM_FRAMES];
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	VkPipelineLayout m_gfx_pipeline_layout;
	VulkanShader m_csm_shader;
	VkPipeline m_gfx_pipeline;
	/* Orthographic projection matrices for each cascade */
	float interp_factor = 0.9f;
	float frustum_near = 0.1f;
	float frustum_far  = 100.0f;

	void compute_z_splits();
	struct push_constants
	{
		glm::mat4 model;
		glm::mat4 dir_light_view;
	} ps;

	void compute_cascade_ortho_proj();
	void render(size_t current_frame_idx, VkCommandBuffer cmd_buffer);
	void draw_scene(VkCommandBuffer cmd_buffer);
	static inline Buffer proj_mats_ubo;
	static inline size_t mats_ubo_size_bytes = 0;

	static inline Buffer cascade_ends_ubo;
	static inline size_t cascade_ends_ubo_size_bytes = 0;

	glm::ivec2 m_shadow_map_size;
	const LightManager* h_light_manager;
	const Camera* h_camera;
	static inline std::array<float, NUM_CASCADES> z_splits;
	std::array<glm::mat4, NUM_CASCADES> camera_splits_proj_mats;
	std::array<glm::mat4, NUM_CASCADES> cascades_proj_mats;

};