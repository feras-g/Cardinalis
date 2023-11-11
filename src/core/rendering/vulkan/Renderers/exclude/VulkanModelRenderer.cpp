#include "VulkanModelRenderer.h"
#include "core/rendering/vulkan/VulkanDebugUtils.h"

#include <glm/mat4x4.hpp>

const uint32_t num_storage_buffers = 2;
const uint32_t num_uniform_buffers = 2;
const uint32_t num_combined_image_smp = 0;

VulkanModelRenderer::VulkanModelRenderer()
{
	m_shader.create("DeferrredGeometryPass.vert.spv", "DeferrredGeometryPass.frag.spv");
	
	/* Render objects creation */
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_uniform_buffers},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, num_storage_buffers}
	};
	m_descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

	/* Configure layout for descriptor set used for a Pass */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	//bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, (uint32_t)RenderObjectManager::textures.size(), VK_SHADER_STAGE_FRAGMENT_BIT });  /* Mesh Textures array */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 3, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 4, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */

	m_pass_descriptor_set_layout = create_descriptor_set_layout(bindings);
	m_pass_descriptor_set = create_descriptor_set(m_descriptor_pool, m_pass_descriptor_set_layout);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	//desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);		/* Per mesh geometry data */
	//desc_set_layouts.push_back(RenderObjectManager::drawable_descriptor_set_layout);	/* Per object data */
	desc_set_layouts.push_back(m_pass_descriptor_set_layout);							/* Per pass */
	desc_set_layouts.push_back(VulkanRendererCommon::get_instance().m_framedata_desc_set_layout.vk_set_layout);	/* Per frame */

	VkPushConstantRange pushConstantRanges[2] =
	{
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size   = sizeof(glm::mat4)
		},
		{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = sizeof(glm::mat4),
			//.size = sizeof(Material)
		}
	};

	m_ppl_layout = create_pipeline_layout(context.device, desc_set_layouts, pushConstantRanges);

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		update_descriptor_set(context.device, frame_idx);
	}
	
	/* Dynamic renderpass setup */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(VulkanRendererCommon::get_instance().m_gbuffer_albedo[i].view);
		m_dyn_renderpass[i].add_color_attachment(VulkanRendererCommon::get_instance().m_gbuffer_normal[i].view);
		m_dyn_renderpass[i].add_color_attachment(VulkanRendererCommon::get_instance().m_gbuffer_metallic_roughness[i].view);
		m_dyn_renderpass[i].add_depth_attachment(VulkanRendererCommon::get_instance().m_gbuffer_depth[i].view);
	}

	Pipeline::Flags ppl_flags = Pipeline::Flags::ENABLE_DEPTH_STATE ;
	std::array<VkFormat, 3> color_attachment_formats
	{
		VulkanRendererCommon::get_instance().tex_base_color_format,
		VulkanRendererCommon::get_instance().tex_gbuffer_normal_format,
		VulkanRendererCommon::get_instance().tex_metallic_roughness_format
	};
	//Pipeline::create_graphics_pipeline_dynamic(m_shader, color_attachment_formats, VulkanRendererBase::tex_gbuffer_depth_format, ppl_flags, m_ppl_layout, &m_gfx_pipeline, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void VulkanModelRenderer::render(size_t currentImageIdx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred G-Buffer Pass");

	/* Transition to write */
	VulkanRendererCommon::get_instance().m_gbuffer_albedo[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_normal[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_depth[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_metallic_roughness[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	set_viewport_scissor(cmd_buffer, VulkanRendererCommon::get_instance().render_width, VulkanRendererCommon::get_instance().render_height, true);

	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 2, 1, &m_pass_descriptor_set, 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 3, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[currentImageIdx].vk_set, 0, nullptr);
	m_dyn_renderpass[currentImageIdx].begin(cmd_buffer, { VulkanRendererCommon::get_instance().render_width , VulkanRendererCommon::get_instance().render_height });

	//for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	//{
	//	Drawable& drawable = RenderObjectManager::drawables[i];
	//	if (drawable.is_visible())
	//	{
	//		draw_scene(cmd_buffer, currentImageIdx, drawable);
	//	}
	//}
	
	m_dyn_renderpass[currentImageIdx].end(cmd_buffer);

	/* Transition to read */
	VulkanRendererCommon::get_instance().m_gbuffer_albedo[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_normal[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_depth[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	VulkanRendererCommon::get_instance().m_gbuffer_metallic_roughness[currentImageIdx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}

//void VulkanModelRenderer::draw_scene(VkCommandBuffer cmdBuffer, size_t current_frame_idx, Drawable& drawable)
//{
	//assert(drawable.get_mesh().descriptor_set);
	/* Mesh descriptor set : per mesh geometry data */
	//vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 0, 1, &drawable.get_mesh().descriptor_set, 0, nullptr);

	/* Object descriptor set : per instance data */
	//uint32_t dynamic_offset  = static_cast<uint32_t>(drawable.id * RenderObjectManager::per_object_data_dynamic_aligment);
	//vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 1, 1, &RenderObjectManager::drawable_descriptor_set, 1, &dynamic_offset);
	
	//if (drawable.get_mesh().geometry_data.primitives.size())
	//{
	//	for (int prim_idx = 0; prim_idx < drawable.get_mesh().geometry_data.primitives.size(); prim_idx++)
	//	{
	//		const Primitive& p = drawable.get_mesh().geometry_data.primitives[prim_idx];

	//		/* Model matrix push constant */
	//		glm::mat4 model_mat = drawable.transform.model * p.mat_model;
	//		vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_mat);
	//		/* Material ID push constant */
	//		//vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), &RenderObjectManager::materials[p.material_id]);
	//		vkCmdDraw(cmdBuffer, p.index_count, 1, p.first_index, 0);
	//	}
	//}
	//else
	//{
	//	vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &drawable.get_mesh().model);
	//	//vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), &RenderObjectManager::materials[drawable.material_id]);
	//	drawable.draw(cmdBuffer);
	//}
	//else
	//{
	//	vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material), &RenderObjectManager::materials[drawable.material_id]);
	//	drawable.draw(cmdBuffer);
	//}
//}

void VulkanModelRenderer::update(size_t frame_idx, const VulkanRendererCommon::FrameData& frame_data)
{
	//upload_buffer_data(VulkanRendererBase::m_ubo_framedata[frame_idx], &frame_data, sizeof(VulkanRendererBase::PerFrameData), 0);
}

void VulkanModelRenderer::update_descriptor_set(VkDevice device, size_t frame_idx)
{
	std::vector<VkWriteDescriptorSet> desc_writes;

	/* Textures array */
	std::vector<VkDescriptorImageInfo> tex_descriptors;
	//for (size_t i = 0; i < RenderObjectManager::textures.size(); i++)
	//{
	//	tex_descriptors.push_back({ .imageView = RenderObjectManager::textures[i].view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	//}

	texture_array_write_descriptor_set =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = m_pass_descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = static_cast<uint32_t>(tex_descriptors.size()),
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = tex_descriptors.data()
	};
	desc_writes.push_back(texture_array_write_descriptor_set);

	
	VkDescriptorImageInfo sampler_info = {};
	sampler_info.sampler = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 1, sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererCommon::get_instance().s_SamplerClampLinear;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 2, sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatNearest;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 3, sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatLinear;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 4, sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));


	vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
}

VulkanModelRenderer::~VulkanModelRenderer()
{
}
