#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/CubemapRenderer.h"
#include "Rendering/Vulkan/Renderers/ShadowRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"

const uint32_t num_descriptors = 4;

void DeferredRenderer::init(std::span<Texture2D> g_buffers_shadow_map, const LightManager& light_manager)
{
	h_light_manager = &light_manager;
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		m_g_buffers_shadow_map[i] = &g_buffers_shadow_map[i];
	}

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(VulkanRendererBase::m_deferred_lighting_attachment[i].view);
	}

	/* Create shader */
	m_shader_deferred.create("DeferredLightingPass.vert.spv", "DeferredLightingPass.frag.spv");

	/* Create Descriptor Pool */
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
	};
	m_descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

	/* Create descriptor set bindings */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatLinear }); /* Color */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatNearest }); /* Normal */
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatNearest }); /* Depth */
	bindings.push_back({ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatNearest }); /* Normal map */
	bindings.push_back({ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatNearest }); /* Metallic/Roughness */
	bindings.push_back({ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatNearest }); /* Shadow Map */
	bindings.push_back({ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatLinear }); /*  Cube map */
	bindings.push_back({ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatLinear }); /*  Irradiance map */
	bindings.push_back({ 8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }); /* Light data */
	bindings.push_back({ 9, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }); /* Shadow cascade info */
	bindings.push_back({ 10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });

	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::array<VkDescriptorSetLayout, 2> layouts =
	{
		m_descriptor_set_layout,
		VulkanRendererBase::m_framedata_desc_set_layout.vk_set_layout
	};

	m_pipeline_layout = create_pipeline_layout(context.device, layouts);

	/* Create Descriptor Set Layout */
	for (size_t frameIdx = 0; frameIdx < NUM_FRAMES; frameIdx++)
	{
		m_descriptor_set[frameIdx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
		update_descriptor_set(frameIdx);
	}

	/* Create graphics pipeline */
	Pipeline::Flags ppl_flags = Pipeline::NONE;
	std::array<VkFormat, 1> color_formats = { VulkanRendererBase::tex_deferred_lighting_format };
	Pipeline::create_graphics_pipeline_dynamic(m_shader_deferred, color_formats, {}, ppl_flags, m_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void DeferredRenderer::draw_scene(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	set_viewport_scissor(cmd_buffer, VulkanRendererBase::render_width, VulkanRendererBase::render_height);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set[current_backbuffer_idx], 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 1, 1, &VulkanRendererBase::m_framedata_desc_set[current_backbuffer_idx].vk_set, 0, nullptr);
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
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampLinear,  VulkanRendererBase::m_gbuffer_albedo[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, VulkanRendererBase::m_gbuffer_normal[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, VulkanRendererBase::m_gbuffer_depth[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, VulkanRendererBase::m_gbuffer_normal[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }); // TODO : replace with emissive
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, VulkanRendererBase::m_gbuffer_metallic_roughness[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampNearest, m_g_buffers_shadow_map[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampLinear,  CubemapRenderer::tex_cubemap_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	descriptor_image_infos.push_back({ VulkanRendererBase::s_SamplerClampLinear,  CubemapRenderer::tex_irradiance_map_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	std::vector<VkWriteDescriptorSet> write_descriptor_set = {};
	for (size_t i = 0; i < descriptor_image_infos.size(); i++)
	{
		write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], (uint32_t)i, &descriptor_image_infos[i]));
	}

	/* Light data */
	VkDescriptorBufferInfo desc_buffer_info = {};
	desc_buffer_info.buffer = h_light_manager->m_ubo[frame_idx];
	desc_buffer_info.offset = 0;
	desc_buffer_info.range = sizeof(h_light_manager->m_light_data);
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 8, &desc_buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	///* Shadow cascade info */
	VkDescriptorBufferInfo desc_buffer_info_ = {};
	desc_buffer_info_.buffer = CascadedShadowRenderer::cascade_ends_ubo;
	desc_buffer_info_.offset = 0;
	desc_buffer_info_.range  = CascadedShadowRenderer::cascade_splits_ubo_size;
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 9, &desc_buffer_info_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	VkDescriptorBufferInfo desc_buffer_info__ = {};
	desc_buffer_info__.buffer = CascadedShadowRenderer::view_proj_mats_ubo[frame_idx];
	desc_buffer_info__.offset = 0;
	desc_buffer_info__.range = CascadedShadowRenderer::mats_ubo_size_bytes;
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 10, &desc_buffer_info__, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	vkUpdateDescriptorSets(context.device, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);
}

void DeferredRenderer::render(size_t current_backbuffer_idx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");

	VulkanRendererBase::m_deferred_lighting_attachment[current_backbuffer_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	VkRect2D render_area{ .offset {}, .extent { VulkanRendererBase::render_width , VulkanRendererBase::render_height } };
	m_dyn_renderpass[current_backbuffer_idx].begin(cmd_buffer, render_area);
	draw_scene(current_backbuffer_idx, cmd_buffer);
	m_dyn_renderpass[current_backbuffer_idx].end(cmd_buffer);

	VulkanRendererBase::m_deferred_lighting_attachment[current_backbuffer_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}