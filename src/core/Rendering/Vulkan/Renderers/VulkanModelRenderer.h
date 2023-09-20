#pragma once

#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/VulkanMesh.h"
#include "core/rendering/vulkan/VulkanShader.h"
#include "core/rendering/vulkan/RenderPass.h"
#include "core/rendering/vulkan/LightManager.h"

class VulkanModelRenderer 
{
	friend class VulkanImGuiRenderer;
public:
	explicit VulkanModelRenderer();
	void render(size_t currentImageIdx, VkCommandBuffer cmd_buffer);

	void draw_scene(VkCommandBuffer cmdBuffer, size_t current_frame_idx,  Drawable& drawable);
	void update(size_t frame_idx, const VulkanRendererBase::PerFrameData& frame_data);
	void update_descriptor_set(VkDevice device, size_t frame_idx);

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];

	static inline VkWriteDescriptorSet texture_array_write_descriptor_set;

	~VulkanModelRenderer();
private:
	VertexFragmentShader m_shader;

	VkSampler m_TextureSampler;

	VkPipeline			m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_ppl_layout;
	
	VkDescriptorSetLayout m_pass_descriptor_set_layout;
	VkDescriptorSet m_pass_descriptor_set;
};
