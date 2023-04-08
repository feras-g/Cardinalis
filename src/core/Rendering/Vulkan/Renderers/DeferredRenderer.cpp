#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

void DeferredRenderer::init(std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, std::span<Texture2D> g_buffers_depth)
{

	for (int i = 0; i < g_buffers_albedo.size(); i++) m_g_buffers_albedo[i] = &g_buffers_albedo[i];
	for (int i = 0; i < g_buffers_normal.size(); i++) m_g_buffers_normal[i] = &g_buffers_normal[i];
	for (int i = 0; i < g_buffers_depth.size(); i++) m_g_buffers_depth[i] = &g_buffers_depth[i];

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
	m_descriptor_pool = create_descriptor_pool(0, 0, num_gbuffers);

	/* Create descriptor set bindings */
	std::array<VkDescriptorSetLayoutBinding, num_gbuffers> desc_set_layout_bindings = {};
	for (uint32_t binding = 0; binding < num_gbuffers; binding++)
	{
		desc_set_layout_bindings[binding] = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest };
	}

	/* Create Descriptor Set Layout */
	m_descriptor_set_layout = create_descriptor_set_layout(desc_set_layout_bindings);

	m_descriptor_set = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);

	/*update_descriptor_set();*/

	m_pipeline_layout = create_pipeline_layout(context.device, m_descriptor_set_layout);

	/* Create graphics pipeline */
	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::NONE;
	std::array<VkFormat, 1> color_formats = { color_attachment_format };
	GraphicsPipeline::CreateDynamic(m_shader_deferred, color_formats, {}, ppl_flags, m_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void DeferredRenderer::draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	SetViewportScissor(cmd_buffer, VulkanModelRenderer::render_width, VulkanModelRenderer::render_height);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);
	vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
}

void DeferredRenderer::create_uniform_buffers()
{

}

void DeferredRenderer::update(size_t frame_idx)
{
	update_descriptor_set(frame_idx);
}

void DeferredRenderer::update_descriptor_set(size_t frame_idx)
{
	using DescriptorImageInfo_GBuffers = std::array<VkDescriptorImageInfo, num_gbuffers>;
	using WriteDescriptorSets_GBuffers = std::array<VkWriteDescriptorSet, num_gbuffers>;

	DescriptorImageInfo_GBuffers descriptor_image_infos = {};
	WriteDescriptorSets_GBuffers write_descriptor_set = {};

	for (int gbuffer_idx = 0; gbuffer_idx < num_gbuffers; gbuffer_idx++)
	{
		descriptor_image_infos[gbuffer_idx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptor_image_infos[gbuffer_idx].sampler = VulkanRendererBase::s_SamplerClampNearest;

		if (gbuffer_idx == 0)	descriptor_image_infos[gbuffer_idx].imageView = m_g_buffers_albedo[frame_idx]->view;
		if (gbuffer_idx == 1)	descriptor_image_infos[gbuffer_idx].imageView = m_g_buffers_normal[frame_idx]->view;
		if (gbuffer_idx == 2)	descriptor_image_infos[gbuffer_idx].imageView = m_g_buffers_depth[frame_idx]->view;

		write_descriptor_set[gbuffer_idx] = ImageWriteDescriptorSet(m_descriptor_set, gbuffer_idx, &descriptor_image_infos[gbuffer_idx]);
	}
	vkUpdateDescriptorSets(context.device, (uint32_t)(write_descriptor_set.size()), write_descriptor_set.data(), 0, nullptr);
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