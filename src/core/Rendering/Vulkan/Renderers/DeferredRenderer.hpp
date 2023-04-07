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
		@param color_attachments Output attachments for each frame 
	*/
	void init(std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, std::span<Texture2D> g_buffers_depth);
	void render(size_t current_backbuffer_idx);

	VulkanShader m_shader_deferred;
	Texture2D color_attachments[NUM_FRAMES];

	VkPipeline m_gfx_pipeline;
	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
};

void DeferredRenderer::init(std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, std::span<Texture2D> g_buffers_depth)
{
#if 0
	/* Create renderpass */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view);
	}

	/* Create shader */
	m_shader_deferred.LoadFromFile("Deferred.vert.spv", "Deferred.frag.spv");

	/* Create descriptor sets/layout/bindings */
	std::array<VkDescriptorSetLayoutBinding, num_gbuffers> desc_set_layout_bindings = {};
	for (uint32_t binding = 0; binding < num_gbuffers; binding++)
	{
		desc_set_layout_bindings[binding] = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, VulkanRendererBase::s_SamplerClampNearest };
	}

	VkDescriptorPoolSize pool_size = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_gbuffers };
	VkDescriptorPoolCreateInfo desc_pool_info
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = NUM_FRAMES,
		.poolSizeCount = 1,
		.pPoolSizes = &pool_size
	};

	VkDescriptorPool desc_pool;
	VK_CHECK(vkCreateDescriptorPool(context.device, &desc_pool_info, nullptr, &desc_pool));
	
	VkDescriptorSetLayoutCreateInfo desc_set_layout_info
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = num_gbuffers,
		.pBindings = desc_set_layout_bindings.data()
	};
	VkDescriptorSetLayout desc_set_layout;
	VK_CHECK(vkCreateDescriptorSetLayout(context.device, &desc_set_layout_info, nullptr, &desc_set_layout));


	VkDescriptorSetAllocateInfo desc_set_alloc_info = {};
	desc_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_alloc_info.descriptorSetCount = NUM_FRAMES;
	desc_set_alloc_info.descriptorPool = desc_pool;
	desc_set_alloc_info.pSetLayouts = &desc_set_layout;

	std::array<VkDescriptorSet, NUM_FRAMES> desc_sets = {};
	VK_CHECK(vkAllocateDescriptorSets(context.device, &desc_set_alloc_info, desc_sets.data()));

	std::array<VkDescriptorImageInfo, 3> desc_image_info_frame0;
	std::array<VkDescriptorImageInfo, 3> desc_image_info_frame1;

	std::array<uint32_t, 3> g_buffer_indices_frame0 = { 0, 2, 0 };
	std::array<uint32_t, 3> g_buffer_indices_frame1 = { 1, 3, 1 };
	for (int i = 0; i < desc_image_info_frame0.size(); i++)
	{
		desc_image_info_frame0[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc_image_info_frame0[i].sampler = VulkanRendererBase::s_SamplerClampNearest;

		desc_image_info_frame1[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc_image_info_frame1[i].sampler = VulkanRendererBase::s_SamplerClampNearest;
	}

	//VkDescriptorImageInfo desc_image_info;
	//desc_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//desc_image_info.imageView = 

	//VkWriteDescriptorSet write_desc_set;
	//write_desc_set.

	VkPipelineLayoutCreateInfo ppl_layout_info
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &desc_set_layout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = VK_NULL_HANDLE
	};

	VkPipelineLayout ppl_layout;
	VK_CHECK(vkCreatePipelineLayout(context.device, &ppl_layout_info, nullptr, &ppl_layout));

	/* Create graphics pipeline */
	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::DISABLE_VTX_INPUT_STATE;
	GraphicsPipeline::CreateDynamic(m_shader_deferred, { color_attachment_format }, {}, ppl_flags, ppl_layout,&m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
#endif
}

void DeferredRenderer::render(size_t current_backbuffer_idx)
{

}