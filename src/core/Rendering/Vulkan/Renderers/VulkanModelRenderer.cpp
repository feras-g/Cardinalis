#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

#include <glm/mat4x4.hpp>

Buffer VulkanModelRenderer::m_uniform_buffer;

const uint32_t num_storage_buffers = 2;
const uint32_t num_uniform_buffers = 2;
const uint32_t num_combined_image_smp = 0;

VulkanModelRenderer::VulkanModelRenderer()
{
	m_Shader.reset(new VulkanShader("Model.vert.spv", "Model.frag.spv"));

	/* Render objects creation */
	create_attachments();
	m_descriptor_pool = create_descriptor_pool(num_storage_buffers, num_uniform_buffers, num_combined_image_smp, 0);
	create_buffers();

	/* Configure layout for descriptor set used for a Pass */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT });  /* Frame data */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, (uint32_t)RenderObjectManager::textures.size(), VK_SHADER_STAGE_FRAGMENT_BIT });  /* Texture array */
	
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 3, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 4, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */
	bindings.push_back({ 5, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });  /* Sampler */

	m_pass_descriptor_set_layout = create_descriptor_set_layout(bindings);
	m_pass_descriptor_set = create_descriptor_set(m_descriptor_pool, m_pass_descriptor_set_layout);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);		/* Per mesh geometry data */
	desc_set_layouts.push_back(RenderObjectManager::drawable_descriptor_set_layout);	/* Per object data */
	desc_set_layouts.push_back(m_pass_descriptor_set_layout);							/* Per pass */
	m_ppl_layout = create_pipeline_layout(context.device, desc_set_layouts, 0,  /* material push constant */ sizeof(Material));

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		update_descriptor_set(context.device, frame_idx);
	}
	

	/* Dynamic renderpass setup */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(m_Albedo_Output[i].view);
		m_dyn_renderpass[i].add_color_attachment(m_Normal_Output[i].view);
		m_dyn_renderpass[i].add_color_attachment(m_MetallicRoughness_Output[i].view);
		m_dyn_renderpass[i].add_color_attachment(m_NormalMap_Output[i].view);
		m_dyn_renderpass[i].add_depth_attachment(m_Depth_Output[i].view);
	}

	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::Flags::ENABLE_DEPTH_STATE;
	GraphicsPipeline::CreateDynamic(*m_Shader.get(), m_ColorAttachmentFormats, m_DepthAttachmentFormat, ppl_flags, m_ppl_layout, &m_gfx_pipeline, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void VulkanModelRenderer::render(size_t currentImageIdx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred G-Buffer Pass");

	/* Transition */
	m_Albedo_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_Normal_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_Depth_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_NormalMap_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_MetallicRoughness_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { render_width , render_height } };
	m_dyn_renderpass[currentImageIdx].begin(cmd_buffer, render_area);
	draw_scene(cmd_buffer, currentImageIdx);
	m_dyn_renderpass[currentImageIdx].end(cmd_buffer);

	/* Transition */
	m_Albedo_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Normal_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Depth_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_NormalMap_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_MetallicRoughness_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanModelRenderer::draw_scene(VkCommandBuffer cmdBuffer, size_t current_frame_idx)
{
	SetViewportScissor(cmdBuffer, render_width, render_height, true);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	
	/* Update UBO descriptor for frame data */
	VkDescriptorBufferInfo ubo_framedata_desc_info = { .buffer = VulkanRendererBase::m_ubo_common_framedata[current_frame_idx].buffer, .offset = 0, .range = sizeof(VulkanRendererBase::PerFrameData) };

	/* Bind Pass descriptor set */
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 2, 1, &m_pass_descriptor_set, 0, nullptr);
	
	for (size_t i=0; i < RenderObjectManager::drawables.size(); i++)
	{
		const Drawable& drawable = RenderObjectManager::drawables[i];

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 0, 1, &drawable.mesh_handle->descriptor_set, 0, nullptr);
		uint32_t dynamic_offset  = i * RenderObjectManager::drawable_data_dynamic_aligment;
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 1, 1, &RenderObjectManager::drawable_descriptor_set, 1, &dynamic_offset);

		if (drawable.has_primitives)
		{
			for (int i = 0; i < drawable.mesh_handle->geometry_data.primitives.size(); i++)
			{
				const Primitive& p = drawable.mesh_handle->geometry_data.primitives[i];
				vkCmdPushConstants(cmdBuffer, m_ppl_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material), &RenderObjectManager::materials[p.material_id]);
				//upload_buffer_data(RenderObjectManager::material_data_ubo, &RenderObjectManager::materials[p.material_id], sizeof(Material), 0);
				vkCmdDraw(cmdBuffer, p.index_count, 1, p.first_index, 0);
			}
		}
		else
		{
			drawable.draw(cmdBuffer);
		}
	}
}

void VulkanModelRenderer::update_buffers(const VulkanRendererBase::PerFrameData& frame_data)
{
	
}

void VulkanModelRenderer::update_descriptor_set(VkDevice device, size_t frame_idx)
{
	std::vector<VkWriteDescriptorSet> desc_writes;

	/* Frame data UBO */
	VulkanRendererBase::PerFrameData frame_data = {};
	upload_buffer_data(VulkanRendererBase::m_ubo_common_framedata[frame_idx], &frame_data, sizeof(VulkanRendererBase::PerFrameData), 0);

	VkDescriptorBufferInfo info = { VulkanRendererBase::m_ubo_common_framedata[frame_idx].buffer,  0, sizeof(VulkanRendererBase::PerFrameData) };
	desc_writes.push_back(BufferWriteDescriptorSet(m_pass_descriptor_set, 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	/* Textures array */
	std::vector<VkDescriptorImageInfo> tex_descriptors;
	for (size_t i = 0; i < RenderObjectManager::textures.size(); i++)
	{
		tex_descriptors.push_back({ .imageView = RenderObjectManager::textures[i].view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	}
	VkWriteDescriptorSet tex_array_desc =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = m_pass_descriptor_set,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = static_cast<uint32_t>(tex_descriptors.size()),
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = tex_descriptors.data()
	};
	desc_writes.push_back(tex_array_desc);

	
	VkDescriptorImageInfo sampler_info = {};
	sampler_info.sampler = VulkanRendererBase::s_SamplerClampNearest;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 2, &sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererBase::s_SamplerClampLinear;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 3, &sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererBase::s_SamplerRepeatNearest;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 4, &sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));

	sampler_info.sampler = VulkanRendererBase::s_SamplerRepeatLinear;
	desc_writes.push_back(ImageWriteDescriptorSet(m_pass_descriptor_set, 5, &sampler_info, VK_DESCRIPTOR_TYPE_SAMPLER));


	vkUpdateDescriptorSets(context.device, desc_writes.size(), desc_writes.data(), 0, nullptr);
}

void VulkanModelRenderer::create_attachments()
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	/* Write Attachments */
	/* Create G-Buffers for Albedo, Normal, Depth for each Frame */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		std::string s_prefix = "Frame #" + std::to_string(i) + "G-Buffer ";

		m_Albedo_Output[i].init(m_ColorAttachmentFormats[0], render_width, render_height, false);	/* G-Buffer Color */
		m_Normal_Output[i].init(m_ColorAttachmentFormats[1], render_width, render_height, false);	/* G-Buffer Normal */
		m_Depth_Output[i].init(m_DepthAttachmentFormat, render_width, render_height, false);		/* G-Buffer Depth */
		m_NormalMap_Output[i].init(m_ColorAttachmentFormats[2], render_width, render_height, false);			/* G-Buffer Normal map */
		m_MetallicRoughness_Output[i].init(m_ColorAttachmentFormats[3], render_width, render_height, false);	/* G-Buffer Metallic/Roughness */

		m_Albedo_Output[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Albedo").c_str());
		m_Normal_Output[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Normal").c_str());
		m_Depth_Output[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Depth").c_str());
		m_NormalMap_Output[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Normal map").c_str());
		m_MetallicRoughness_Output[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Metallic roughness").c_str());

		m_Albedo_Output[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_Normal_Output[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_Depth_Output[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_NormalMap_Output[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_MetallicRoughness_Output[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });

		m_Albedo_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_Normal_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_Depth_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_NormalMap_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_MetallicRoughness_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	
	end_temp_cmd_buffer(cmd_buffer);
}


void VulkanModelRenderer::create_buffers()
{
	
}

VulkanModelRenderer::~VulkanModelRenderer()
{
	vkDeviceWaitIdle(context.device);
	
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_Albedo_Output[i].destroy(context.device);
		m_Normal_Output[i].destroy(context.device);
		m_Depth_Output[i].destroy(context.device);
	}
}
