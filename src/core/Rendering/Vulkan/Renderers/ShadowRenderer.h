#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include <span>

struct Drawable;
struct LightManager;
struct Texture2D;
class Camera;

static const uint32_t view_mask = 0b00001111;

struct CascadedShadowRenderer
{
	void init(unsigned int width, unsigned int height,  Camera& camera, const LightManager& lightmanager);
	void update_desc_sets();
	void create_buffers();

	static constexpr int NUM_CASCADES = 4;

	Texture2D m_shadow_maps[NUM_FRAMES];
	vk::DynamicRenderPass m_CSM_pass[NUM_FRAMES];
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	VkPipelineLayout m_gfx_pipeline_layout;
	VertexFragmentShader m_csm_shader;
	VkPipeline m_gfx_pipeline;
	/* Orthographic projection matrices for each cascade */
	float lambda = 0.9f;

	float frustum_near = 16.0f;
	float frustum_far  = 100.0f;

	struct push_constants
	{
		glm::mat4 model;
	} ps;

	void compute_cascade_splits();
	void compute_cascade_ortho_proj(size_t current_frame_idx);
	void render(size_t current_frame_idx, VkCommandBuffer cmd_buffer);
	void draw_scene(VkCommandBuffer cmd_buffer);
	static inline Buffer view_proj_mats_ubo[NUM_FRAMES];
	static inline Buffer cascade_ends_ubo;

	static inline size_t mats_ubo_size_bytes = 0;

	static inline size_t cascade_splits_ubo_size = 0;
	glm::ivec2 m_shadow_map_size;
	const LightManager* h_light_manager;
	Camera* h_camera;
	static inline std::array<float, NUM_CASCADES> cascade_splits;
	std::array<glm::mat4, NUM_CASCADES> view_proj_mats;
	std::array<glm::mat4, NUM_CASCADES> projection_for_cascade;

	float clip_range;
	float near_clip;
	float far_clip;

	std::array<glm::mat4, 4> per_cascade_projection;
	bool b_use_per_cascade_projection = false;
	/* 
		For each frame, hold separate views for each layer of a texture 2D array.
		To be displayed by ImGui. 
	*/
	static inline std::array<std::array<VkImageView, NUM_CASCADES>, NUM_FRAMES> cascade_views;
};