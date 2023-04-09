#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

const uint32_t num_descriptors = 4;

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

	/* Create Descriptor Pool */
	m_descriptor_pool = create_descriptor_pool(0, 0, num_descriptors);

	/* Create descriptor set bindings */
	std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {};
	desc_set_layout_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest });
	desc_set_layout_bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest });
	desc_set_layout_bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerClampNearest });
	desc_set_layout_bindings.push_back({ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });
	desc_set_layout_bindings.push_back({ 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });

	/* Create Descriptor Set Layout */
	m_descriptor_set_layout = create_descriptor_set_layout(desc_set_layout_bindings);

	m_descriptor_set = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);

	m_pipeline_layout = create_pipeline_layout(context.device, m_descriptor_set_layout);

	m_light_manager.init(10, 10);


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
	
}

void DeferredRenderer::update_descriptor_set(size_t frame_idx)
{
	std::array<VkDescriptorImageInfo, num_gbuffers>  descriptor_image_infos = {};
	std::vector<VkWriteDescriptorSet> write_descriptor_set = {};

	/* GBuffers */
	descriptor_image_infos[0].imageView = m_g_buffers_albedo[frame_idx]->view;
	descriptor_image_infos[1].imageView = m_g_buffers_normal[frame_idx]->view;
	descriptor_image_infos[2].imageView = m_g_buffers_depth[frame_idx]->view;

	for (int gbuffer_idx = 0; gbuffer_idx < num_gbuffers; gbuffer_idx++)
	{
		descriptor_image_infos[gbuffer_idx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptor_image_infos[gbuffer_idx].sampler = VulkanRendererBase::s_SamplerClampNearest;
	}

	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set, 0, &descriptor_image_infos[0]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set, 1, &descriptor_image_infos[1]));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(m_descriptor_set, 2, &descriptor_image_infos[2]));

	/* Frame data : MVP matrices */
	VkDescriptorBufferInfo desc_buffer_info = {};
	desc_buffer_info.buffer = VulkanRendererBase::m_ubo_common_framedata.buffer;
	desc_buffer_info.offset = 0;
	desc_buffer_info.range = sizeof(VulkanRendererBase::UniformData);
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set, 3, &desc_buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

	/* Light data */
	VkDescriptorBufferInfo desc_buffer_info_ = {};
	desc_buffer_info_.buffer = m_light_manager.m_uniform_buffer.buffer;
	desc_buffer_info_.offset = 0;
	desc_buffer_info_.range = m_light_manager.m_total_size_bytes;
	write_descriptor_set.push_back(BufferWriteDescriptorSet(m_descriptor_set, 4, &desc_buffer_info_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

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

/* Light manager */
void LightManager::init(size_t w, size_t h)
{
	size_t total_size = w * h;

	for (size_t x = 0; x < w; x++)
	{
		for (size_t y = 0; y < h; y++)
		{
			size_t idx = (y * w) + x;

			m_light_data.point_lights[idx].position = glm::vec4(x, y, 50, 1.0);
			m_light_data.point_lights[idx].color = glm::vec4(
				0.5f + (float)(rand() % 129) / 128.0f, 
				0.5f + (float)(rand() % 129) / 128.0f, 
				0.5f + (float)(rand() % 129) / 128.0f,
				1.0);
			m_light_data.point_lights[idx].radius = 10;
		}
	}

	/* Create and fill UBO data */
	init_ubo();
}

void LightManager::init_ubo()
{
	m_total_size_bytes = 100 * sizeof(PointLight);
	CreateBuffer(m_uniform_buffer, m_total_size_bytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	UploadBufferData(m_uniform_buffer, &m_light_data, m_total_size_bytes, 0);
}
