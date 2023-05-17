#include "VulkanRendererBase.h"

VkSampler VulkanRendererBase::s_SamplerRepeatLinear;
VkSampler VulkanRendererBase::s_SamplerClampLinear;
VkSampler VulkanRendererBase::s_SamplerClampNearest;
VkSampler VulkanRendererBase::s_SamplerRepeatNearest;
Buffer VulkanRendererBase::m_ubo_framedata[NUM_FRAMES];

/** Size in pixels of the offscreen buffers */
uint32_t VulkanRendererBase::render_width = 2048;
uint32_t VulkanRendererBase::render_height = 2048;


	
void VulkanRendererBase::create_descriptor_sets()
{
	VkDescriptorPool pool = create_descriptor_pool(0, 1, 0, 0);
	m_framedata_desc_set_layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "Framedata UBO");
	m_framedata_desc_set_layout.create("Framedata layout");

	/* Frame data UBO */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_framedata_desc_set[frame_idx].assign_layout(m_framedata_desc_set_layout);
		m_framedata_desc_set[frame_idx].create(pool, "Framedata descriptor set");
		
		VkDescriptorBufferInfo info = { m_ubo_framedata[frame_idx].buffer,  0, sizeof(VulkanRendererBase::PerFrameData) };
		VkWriteDescriptorSet write = BufferWriteDescriptorSet(m_framedata_desc_set[frame_idx].set, 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
	}
}

void VulkanRendererBase::create_samplers()
{
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatNearest);
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}

void VulkanRendererBase::create_buffers()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		create_uniform_buffer(m_ubo_framedata[frame_idx], sizeof(PerFrameData));
	}
}

void VulkanRendererBase::update_frame_data(const PerFrameData& data, size_t current_frame_idx)
{
	upload_buffer_data(m_ubo_framedata[current_frame_idx], (void*)&data, sizeof(data), 0);
}

void VulkanRendererBase::destroy()
{
	vkDeviceWaitIdle(context.device);
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		destroy_buffer(m_ubo_framedata[i]);
	}
	vkDestroySampler(context.device, s_SamplerRepeatLinear, nullptr);
	vkDestroySampler(context.device, s_SamplerClampLinear, nullptr);
	vkDestroySampler(context.device, s_SamplerClampNearest, nullptr);
	vkDestroySampler(context.device, s_SamplerRepeatNearest, nullptr);
}

void VulkanRendererBase::create_attachments()
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	/* Write Attachments */
	/* Create G-Buffers for Albedo, Normal, Depth for each Frame */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		std::string s_prefix = "Frame #" + std::to_string(i) + "G-Buffer ";

		m_gbuffer_albdedo[i].init(m_formats[0], render_width, render_height, 1, false);	/* G-Buffer Color */
		m_gbuffer_normal[i].init(m_formats[1], render_width, render_height, 1, false);	/* G-Buffer Normal */
		m_gbuffer_depth[i].init(m_depth_format, render_width, render_height, 1, false);		/* G-Buffer Depth */
		m_gbuffer_directional_shadow[i].init(m_depth_format, render_width, render_height, 1, false);			/* G-Buffer Shadow map */
		m_gbuffer_metallic_roughness[i].init(m_formats[2], render_width, render_height, 1, false);			/* G-Buffer Metallic/Roughness */
		m_output_attachment[i].init(color_attachment_format, 2048, 2048, 1, false); /* Final color attachment */

		m_gbuffer_albdedo[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Albedo").c_str());
		m_gbuffer_normal[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Normal").c_str());
		m_gbuffer_depth[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Depth").c_str());
		m_gbuffer_directional_shadow[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Directional Shadow Map").c_str());
		m_gbuffer_metallic_roughness[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Metallic roughness").c_str());
		m_output_attachment[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Deferred Renderer Attachment");

		m_gbuffer_albdedo[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_gbuffer_normal[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_gbuffer_depth[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_gbuffer_directional_shadow[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_gbuffer_metallic_roughness[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_output_attachment[i].create_view(context.device, {});

		m_gbuffer_albdedo[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_gbuffer_normal[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_gbuffer_depth[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_gbuffer_directional_shadow[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_gbuffer_metallic_roughness[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_output_attachment[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	end_temp_cmd_buffer(cmd_buffer);
}
