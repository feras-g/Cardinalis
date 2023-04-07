#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

void DeferredRenderer::init(std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, std::span<Texture2D> g_buffers_depth)
{
	/* Create renderpass */

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_output_attachment[i].info = { color_attachment_format, 1024, 1024, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED };
		m_output_attachment[i].CreateImage(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Deferred Renderer Attachment");
		m_output_attachment[i].CreateView(context.device, {});
		m_output_attachment[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	end_temp_cmd_buffer(cmd_buffer);

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(m_output_attachment[i].view);
	}

	/* Create shader */
	m_shader_deferred.LoadFromFile("Deferred.vert.spv", "Deferred.frag.spv");

	// Descriptor Pool 
	// Descriptor Set Layout Bindings
	// Descriptot Set Layout

	/* Create Descriptor Pool */
	VkDescriptorPoolSize pool_size = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_gbuffers * NUM_FRAMES };
	VkDescriptorPoolCreateInfo desc_pool_info
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = NUM_FRAMES,
		.poolSizeCount = 1,
		.pPoolSizes = &pool_size
	};

	VkDescriptorPool desc_pool;
	VK_CHECK(vkCreateDescriptorPool(context.device, &desc_pool_info, nullptr, &desc_pool));


	/* Create descriptor set bindings */
	std::array<VkDescriptorSetLayoutBinding, num_gbuffers> desc_set_layout_bindings = {};
	for (uint32_t binding = 0; binding < num_gbuffers; binding++)
	{
		desc_set_layout_bindings[binding] = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest };
	}

	/* Create Descriptor Set Layout */
	VkDescriptorSetLayout desc_set_layout[NUM_FRAMES];
	VkDescriptorSetLayoutCreateInfo desc_set_layout_info = {};
	desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_info.bindingCount = desc_set_layout_bindings.size();
	desc_set_layout_info.pBindings = desc_set_layout_bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(context.device, &desc_set_layout_info, nullptr, &desc_set_layout[0]));
	VK_CHECK(vkCreateDescriptorSetLayout(context.device, &desc_set_layout_info, nullptr, &desc_set_layout[1]));

	VkDescriptorSetAllocateInfo desc_set_alloc_info = {};
	desc_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_alloc_info.descriptorSetCount = NUM_FRAMES;
	desc_set_alloc_info.descriptorPool = desc_pool;
	desc_set_alloc_info.pSetLayouts = desc_set_layout;
	VK_CHECK(vkAllocateDescriptorSets(context.device, &desc_set_alloc_info, m_descriptor_sets.data()));

	std::array<VkDescriptorImageInfo, 3> desc_image_info_frame0;
	std::array<VkDescriptorImageInfo, 3> desc_image_info_frame1;

	for (int i = 0; i < desc_image_info_frame0.size(); i++)
	{
		desc_image_info_frame0[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc_image_info_frame0[i].sampler = VulkanRendererBase::s_SamplerClampNearest;

		desc_image_info_frame1[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc_image_info_frame1[i].sampler = VulkanRendererBase::s_SamplerClampNearest;
	}

	desc_image_info_frame0[0].imageView = g_buffers_albedo[0].view;
	desc_image_info_frame0[1].imageView = g_buffers_normal[0].view;
	desc_image_info_frame0[2].imageView = g_buffers_depth[0].view;

	desc_image_info_frame1[0].imageView = g_buffers_albedo[1].view;
	desc_image_info_frame1[1].imageView = g_buffers_normal[1].view;
	desc_image_info_frame1[2].imageView = g_buffers_depth[1].view;

	std::array<VkWriteDescriptorSet, num_gbuffers> desc_writes_frame0
	{
		ImageWriteDescriptorSet(m_descriptor_sets[0], 0, &desc_image_info_frame0[0]),
		ImageWriteDescriptorSet(m_descriptor_sets[0], 1, &desc_image_info_frame0[1]),
		ImageWriteDescriptorSet(m_descriptor_sets[0], 2, &desc_image_info_frame0[2])
	};

	std::array<VkWriteDescriptorSet, num_gbuffers> desc_writes_frame1
	{
		ImageWriteDescriptorSet(m_descriptor_sets[1], 0, &desc_image_info_frame1[0]),
		ImageWriteDescriptorSet(m_descriptor_sets[1], 1, &desc_image_info_frame1[1]),
		ImageWriteDescriptorSet(m_descriptor_sets[1], 2, &desc_image_info_frame1[2])
	};

	vkUpdateDescriptorSets(context.device, (uint32_t)(desc_writes_frame0.size()), desc_writes_frame0.data(), 0, nullptr);
	vkUpdateDescriptorSets(context.device, (uint32_t)(desc_writes_frame1.size()), desc_writes_frame1.data(), 0, nullptr);

	VkPipelineLayoutCreateInfo ppl_layout_info
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = NUM_FRAMES,
		.pSetLayouts = desc_set_layout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = VK_NULL_HANDLE
	};
	VK_CHECK(vkCreatePipelineLayout(context.device, &ppl_layout_info, nullptr, &m_ppl_layout));

	/* Create graphics pipeline */
	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::NONE;
	std::array<VkFormat, 1> color_formats = { color_attachment_format };
	GraphicsPipeline::CreateDynamic(m_shader_deferred, color_formats, {}, ppl_flags, m_ppl_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void DeferredRenderer::draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	SetViewportScissor(cmd_buffer, VulkanModelRenderer::render_width, VulkanModelRenderer::render_height);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 0, 1, &m_descriptor_sets[current_backbuffer_idx], 0, nullptr);
	vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
}

void DeferredRenderer::render(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");

	m_output_attachment[current_backbuffer_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { VulkanModelRenderer::render_width , VulkanModelRenderer::render_height } };
	m_dyn_renderpass[current_backbuffer_idx].begin(cmd_buffer, render_area);
	draw_scene(current_backbuffer_idx, cmd_buffer);
	m_dyn_renderpass[current_backbuffer_idx].end(cmd_buffer);

	m_output_attachment[current_backbuffer_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}