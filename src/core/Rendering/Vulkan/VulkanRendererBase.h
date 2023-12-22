#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <core/rendering/vulkan/VulkanRenderInterface.h>
#include "core/engine/vulkan/objects/vk_renderpass.h"
#include "core/rendering/vulkan/VulkanMesh.h"
#include "core/engine/vulkan/objects/vk_descriptor_set.hpp"

class VulkanRendererCommon
{
private:
	VulkanRendererCommon() = default;
public:
	
	static inline VulkanRendererCommon* s_instance = nullptr;

	static VulkanRendererCommon& get_instance()
	{
		if (nullptr == s_instance)
		{
			s_instance = new VulkanRendererCommon();
			s_instance->init();
		}

		return *s_instance;
	}

	struct FrameData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		glm::mat4 view_proj_inv;
		glm::vec4 camera_pos_ws;
		float time; /* Time in seconds */
	} m_framedata[NUM_FRAMES];
	vk::buffer m_ubo_framedata[NUM_FRAMES];
	vk::descriptor_set_layout m_framedata_desc_set_layout;
	vk::descriptor_set m_framedata_desc_set[NUM_FRAMES];

	void init();
	void create_descriptor_sets();
	void create_samplers();
	void create_buffers();
	void update_frame_data(const FrameData& data, size_t current_frame_idx);
	
	~VulkanRendererCommon();

	VkSampler s_SamplerRepeatLinear;
	VkSampler s_SamplerClampLinear;
	VkSampler s_SamplerClampNearest;
	VkSampler s_SamplerRepeatNearest;

	uint32_t render_width;
	uint32_t render_height;

	/* Color attachment resulting the deferred lighting pass */
	static inline std::array <Texture2D, NUM_FRAMES> m_deferred_lighting_attachment;

	/* Formats */
	VkFormat swapchain_color_format;
	VkColorSpaceKHR swapchain_colorspace;
	VkFormat swapchain_depth_format;
	VkFormat tex_base_color_format;
	VkFormat tex_metallic_roughness_format;
	VkFormat tex_normal_map_format;
	VkFormat tex_emissive_format;
	VkFormat tex_gbuffer_normal_format;
	VkFormat tex_gbuffer_depth_format;
	VkFormat tex_deferred_lighting_format;
};