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
	Texture2D* h_gbuffers_depth[NUM_FRAMES]; // Handles to depth texture written by deferred renderer

	VulkanShader m_shadow_shader;
	vk::DynamicRenderPass m_shadow_pass[NUM_FRAMES];
	
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_gfx_pipeline_layout;
	VkPipeline m_gfx_pipeline;
	const LightManager* h_light_manager;
};

struct CascadedShadowRenderer
{
	/* Orthographic projection matrices for each cascade */
	std::vector<glm::mat4> proj_matrices;
	void compute_cascade_ortho_proj();
	const LightManager* h_light_manager;
	const Camera* h_camera;
};