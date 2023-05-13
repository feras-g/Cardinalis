#include "ShadowRenderer.h"
#include "Rendering/LightManager.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"

static constexpr VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;

void ShadowRenderer::init(unsigned int width, unsigned int height, const LightManager& lightmanager)
{
	h_light_manager = &lightmanager;

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	m_shadow_map_size = { width, height };

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, width, height, false);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_shadow_maps[frame_idx].create_view(context.device, { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT });
		m_shadow_maps[frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* Create render pass */
		m_shadow_pass[frame_idx].add_depth_attachment(m_shadow_maps[frame_idx].view);
	}

	end_temp_cmd_buffer(cmd_buffer);

	/* Shadow pass set */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Lighting data */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Depth G-Buffer */
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Frame data */

	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(m_descriptor_set_layout);								/* Shadow pass descriptor set */
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);		/* Mesh geometry descriptor set*/
	desc_set_layouts.push_back(RenderObjectManager::drawable_descriptor_set_layout);	/* Drawable data descriptor set */

	m_descriptor_pool       = create_descriptor_pool(0, 2, 1, 0);
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_descriptor_set[frame_idx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
	}

	m_gfx_pipeline_layout   = create_pipeline_layout(context.device, desc_set_layouts);

	/* Create graphics pipeline */
	m_shadow_shader.load_from_file("GenShadowMap.vert.spv", "GenShadowMap.frag.spv");
	GfxPipeline::Flags flags = GfxPipeline::Flags::ENABLE_DEPTH_STATE;
	GfxPipeline::CreateDynamic(m_shadow_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void ShadowRenderer::update_desc_sets(std::span<Texture2D*> gbuffers_depth)
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		h_gbuffers_depth[frame_idx] = gbuffers_depth[frame_idx];

		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Lighting data UBO */
		VkDescriptorBufferInfo info = { h_light_manager->m_ubo[frame_idx].buffer,  0, sizeof(LightData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		/* Depth g-buffer*/
		VkDescriptorImageInfo  descriptor_image_info = {};
		descriptor_image_info = { VulkanRendererBase::s_SamplerClampLinear, h_gbuffers_depth[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		desc_writes.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 1, &descriptor_image_info));

		/* Frame data UBO */
		VkDescriptorBufferInfo info2 = { VulkanRendererBase::m_ubo_common_framedata[frame_idx].buffer,  0, sizeof(VulkanRendererBase::PerFrameData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 2, &info2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		vkUpdateDescriptorSets(context.device, desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void ShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { m_shadow_map_size.x , m_shadow_map_size.y } };
	set_viewport_scissor(cmd_buffer, m_shadow_map_size.x, m_shadow_map_size.y, true);
	m_shadow_pass[current_frame_idx].begin(cmd_buffer, render_area);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);


	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);



	draw_scene(cmd_buffer);

	m_shadow_pass[current_frame_idx].end(cmd_buffer);

	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ShadowRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	{
		const Drawable& d = RenderObjectManager::drawables[i];
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &d.mesh_handle->descriptor_set, 0, nullptr);

		/* Object descriptor set : per instance data */
		uint32_t dynamic_offset = d.id * RenderObjectManager::per_object_data_dynamic_aligment;
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 2, 1, &RenderObjectManager::drawable_descriptor_set, 1, &dynamic_offset);

		if (d.has_primitives)
		{
			for (int prim_idx = 0; prim_idx < d.mesh_handle->geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = d.mesh_handle->geometry_data.primitives[prim_idx];
				vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
			}
		}
		else
		{
			d.draw(cmd_buffer);
		}
	}

}

