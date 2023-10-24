#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <core/rendering/vulkan/VulkanRenderInterface.h>
#include <core/rendering/vulkan/RenderPass.h>
#include <core/rendering/vulkan/VulkanMesh.h>
#include "DescriptorSet.h"

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
		glm::mat4 view_proj;
		glm::mat4 view_proj_inv;
	};

	DescriptorSetLayout m_framedata_desc_set_layout;
	DescriptorSet m_framedata_desc_set[NUM_FRAMES];

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
	Buffer m_ubo_framedata[NUM_FRAMES];

	/* WIP */
	VulkanRenderPassDynamic m_dyn_renderpass[NUM_FRAMES];
	VkPipeline m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorSet m_descriptor_set;

	uint32_t render_width;
	uint32_t render_height;

	/* G-Buffers for Deferred rendering */
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_albedo;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_normal;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_depth;
	std::array<Texture2D, NUM_FRAMES> m_gbuffer_metallic_roughness;
	//static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_emissive;

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