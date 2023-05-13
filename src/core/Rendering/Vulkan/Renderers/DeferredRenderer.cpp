#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"


static const VkFormat color_attachment_format = VK_FORMAT_R8G8B8A8_SRGB;

const uint32_t num_descriptors = 4;

void DeferredRenderer::init(
	std::span<Texture2D> g_buffers_albedo, std::span<Texture2D> g_buffers_normal, 
	std::span<Texture2D> g_buffers_depth,  std::span<Texture2D> g_buffers_normal_map,
	std::span<Texture2D> g_buffers_metallic_roughness, std::span<Texture2D> g_buffers_shadow_map, const LightManager& light_manager)
{
	h_light_manager = &light_manager;
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		m_g_buffers_albedo[i] = &g_buffers_albedo[i];
		m_g_buffers_normal[i] = &g_buffers_normal[i];
		m_g_buffers_depth[i]  = &g_buffers_depth[i];
		m_g_buffers_normal_map[i] = &g_buffers_normal_map[i];
		m_g_buffers_metallic_roughness[i] = &g_buffers_metallic_roughness[i];
		m_g_buffers_shadow_map[i] = &g_buffers_shadow_map[i];
	}

	/* Create renderpass */
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_output_attachment[i].init(color_attachment_format, 2048, 2048, 1, false);
		m_output_attachment[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Deferred Renderer Attachment");
		m_output_attachment[i].create_view(context.device, {});
		m_output_attachment[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	end_temp_cmd_buffer(cmd_buffer);

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(m_output_attachment[i].view);
	}

	/* Create shader */
	m_shader_deferred.load_from_file("DeferredLightingPass.vert.spv", "DeferredLightingPass.frag.spv");

	/* Create Descriptor Pool */
	m_descriptor_pool = create_descriptor_pool(0, 0, num_descriptors, 0);

	/* Create descriptor set bindings */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Color */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Normal */
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Depth */
	bindings.push_back({ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Normal map */
	bindings.push_back({ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Metallic/Roughness */
	bindings.push_back({ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest }); /* G-Buffer Shadow Map */

	bindings.push_back({ 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }); /* Frame data */
	bindings.push_back({ 7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }); /* Light data */

	m_descriptor_set_layout = create_descriptor_set_layout(bindings);
	m_pipeline_layout = create_pipeline_layout(context.device, m_descriptor_set_layout);

	/* Create Descriptor Set Layout */
	for (size_t frameIdx = 0; frameIdx < NUM_FRAMES; frameIdx++)
	{
		m_descriptor_set[frameIdx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
		update_descriptor_set(frameIdx);
	}

	/* Create graphics pipeline */
	GfxPipeline::Flags ppl_flags = GfxPipeline::NONE;
	std::array<VkFormat, 1> color_formats = { color_attachment_format };
	GfxPipeline::CreateDynamic(m_shader_deferred, color_formats, {}, ppl_flags, m_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void DeferredRenderer::draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	set_viewport_scissor(cmd_buffer, VulkanModelRenderer::render_width, VulkanModelRenderer::render_height);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set[current_backbuffer_idx], 0, nullptr);
	vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
}

void DeferredRenderer::create_uniform_buffers()
{

}

void DeferredRenderer::update(size_t frame_idx)
{
	//update_descriptor_set(frame_idx);
}

void DeferredRenderer::update_descriptor_set(size_t frame_idx)
{
	/*
		Number of g-buffers to read from :
		0: Albedo / 1: View Space Normal / 2: Depth / 3: Normal map / 4: Metallic/Roughness
	*/
	std::vector<VkDescriptorImageInfo>  descriptor_image_infos = {};
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampLinear, m_g_buffers_albedo[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_normal[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_depth[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_normal_map[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_metallic_roughness[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_shadow_map[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	std::vector<VkWriteDescriptorSet> write_descriptor_set = {};
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &descriptor_image_infos[0]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 1, &descriptor_image_infos[1]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 2, &descriptor_image_infos[2]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 3, &descriptor_image_infos[3]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 4, &descriptor_image_infos[4]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 5, &descriptor_image_infos[5]));

	/* Frame data : MVP matrices */
	VkDescriptorBufferInfo desc_buffer_info = {};
	desc_buffer_info.buffer = VulkanRendererBase::m_ubo_common_framedata[frame_idx].buffer;
	desc_buffer_info.offset = 0;
	desc_buffer_info.range = sizeof(VulkanRendererBase::PerFrameData);
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 6, &desc_buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	/* Light data */
	VkDescriptorBufferInfo desc_buffer_info_ = {};
	desc_buffer_info_.buffer = h_light_manager->m_ubo[frame_idx].buffer;
	desc_buffer_info_.offset = 0;
	desc_buffer_info_.range = sizeof(h_light_manager->m_light_data);
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 7, &desc_buffer_info_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	vkUpdateDescriptorSets(context.device, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);
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