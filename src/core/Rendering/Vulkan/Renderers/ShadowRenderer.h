#pragma once

#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include <span>

struct Drawable;
struct LightManager;
struct Texture2D;
class Camera;

static constexpr uint32_t view_mask = 0b00001111;
static constexpr int NUM_CASCADES = 4;

struct CascadedShadowRenderer
{
	void init(unsigned int width, unsigned int height,  Camera& camera, const LightManager& lightmanager);
	void update_desc_sets();
	void create_buffers();


	static inline Texture2D m_shadow_maps[NUM_FRAMES];
	vk::DynamicRenderPass m_renderpass[NUM_FRAMES];
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	VkPipelineLayout m_gfx_pipeline_layout;
	VertexFragmentShader m_csm_shader;
	VkPipeline m_gfx_pipeline;
	/* Orthographic projection matrices for each cascade */
	float lambda = 0.9f;

	struct push_constants
	{
		glm::mat4 model;
	} ps;

	struct CascadesData
	{
		glm::mat4 view_proj[NUM_CASCADES];
		glm::vec4 splits;					
		glm::uvec4 num_cascades;				
	} cascades_data;

	static inline Buffer cascades_ssbo[NUM_FRAMES];
	static inline size_t cascades_ssbo_size_bytes = 0;

	void compute_cascade_splits();
	void compute_cascade_ortho_proj(size_t current_frame_idx);
	void render(size_t current_frame_idx, VkCommandBuffer cmd_buffer);
	void draw_scene(VkCommandBuffer cmd_buffer);

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