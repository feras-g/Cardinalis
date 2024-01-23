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
	};

	std::array<FrameData, NUM_FRAMES> m_framedata;
	std::array<vk::buffer, NUM_FRAMES> m_ubo_framedata;
	vk::descriptor_set_layout m_framedata_desc_set_layout;
	std::array<vk::descriptor_set, NUM_FRAMES> m_framedata_desc_set;

	void init();
	void create_descriptor_sets();
	void create_samplers();
	void create_buffers();
	void update_frame_data(const FrameData& data, size_t current_frame_idx);
	
	~VulkanRendererCommon();

	VkSampler smp_repeat_linear;
	VkSampler smp_clamp_linear;
	VkSampler smp_clamp_nearest;
	VkSampler smp_repeat_nearest;

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