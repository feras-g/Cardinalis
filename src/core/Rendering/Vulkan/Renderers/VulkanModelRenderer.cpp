#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

#include <glm/mat4x4.hpp>

Buffer VulkanModelRenderer::m_uniform_buffer;

const uint32_t num_storage_buffers = 2;
const uint32_t num_uniform_buffers = 1;
const uint32_t num_combined_image_smp = 1;

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename)
{
	m_Model.CreateFromFile(modelFilename);

	m_Shader.reset(new VulkanShader("Model.vert.spv", "Model.frag.spv"));

	/* Render objects creation */
	create_attachments();
	m_descriptor_pool = create_descriptor_pool(num_storage_buffers, num_uniform_buffers, num_combined_image_smp);
	create_buffers();

	constexpr uint32_t descriptor_count = num_storage_buffers + num_uniform_buffers + num_combined_image_smp;

	std::array<VkDescriptorSetLayoutBinding, descriptor_count> bindings = {};
	bindings[0] = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,VK_SHADER_STAGE_VERTEX_BIT };
	bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_VERTEX_BIT };
	bindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_VERTEX_BIT };
	bindings[3] = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,VK_SHADER_STAGE_FRAGMENT_BIT, &VulkanRendererBase::s_SamplerRepeatLinear };
	
	m_descriptor_set_layout = create_descriptor_set_layout(bindings);
	m_descriptor_set = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);

	update_descriptor_set(context.device);
	m_ppl_layout = create_pipeline_layout(context.device, m_descriptor_set_layout);

	/* Dynamic renderpass setup */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(m_Albedo_Output[i].view);
		m_dyn_renderpass[i].add_color_attachment(m_Normal_Output[i].view);
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

	VkRect2D render_area{ .offset {}, .extent { render_width , render_height } };
	m_dyn_renderpass[currentImageIdx].begin(cmd_buffer, render_area);
	draw_scene(cmd_buffer);
	m_dyn_renderpass[currentImageIdx].end(cmd_buffer);

	/* Transition */
	m_Albedo_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Normal_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Depth_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanModelRenderer::draw_scene(VkCommandBuffer cmdBuffer)
{
	SetViewportScissor(cmdBuffer, render_width, render_height, true);

	// Graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppl_layout, 0, 1, &m_descriptor_set, 0, nullptr);
	m_Model.draw_indexed(cmdBuffer);
}

void VulkanModelRenderer::update_buffers(void* uniform_data, size_t data_size)
{
	
}


void VulkanModelRenderer::update_descriptor_set(VkDevice device)
{
	// Mesh data is not modified between 2 frames
	VkDescriptorBufferInfo sboInfo0 = { .buffer = m_Model.m_StorageBuffer.buffer, .offset = 0, .range = m_Model.m_VtxBufferSizeInBytes };
	VkDescriptorBufferInfo sboInfo1 = { .buffer = m_Model.m_StorageBuffer.buffer, .offset = m_Model.m_VtxBufferSizeInBytes, .range = m_Model.m_IdxBufferSizeInBytes };
	VkDescriptorImageInfo imageInfo0 = { .sampler = VulkanRendererBase::s_SamplerRepeatLinear, .imageView = m_default_texture.view, .imageLayout = m_default_texture.info.imageLayout };

	VkDescriptorBufferInfo uboInfo0 = { .buffer = VulkanRendererBase::m_ubo_common_framedata.buffer, .offset = 0, .range = sizeof(VulkanRendererBase::UniformData) };

	std::array<VkWriteDescriptorSet, 4> descriptorWrites =
	{
		BufferWriteDescriptorSet(m_descriptor_set, 0, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
		BufferWriteDescriptorSet(m_descriptor_set, 1, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		BufferWriteDescriptorSet(m_descriptor_set, 2, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		ImageWriteDescriptorSet(m_descriptor_set,  3,  &imageInfo0)
	};

	vkUpdateDescriptorSets(device, (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanModelRenderer::create_attachments()
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	/* Input attachments */
	m_default_texture.CreateFromFile("../../../data/textures/default.png", "Default Checkerboard Texture", VK_FORMAT_R8G8B8A8_UNORM);
	m_default_texture.CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
	m_default_texture.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	/* Write Attachments */
	/* Create G-Buffers for Albedo, Normal, Depth for each Frame */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		std::string s_prefix = "Frame #" + std::to_string(i) + "G-Buffer ";

		m_Albedo_Output[i].info = { m_ColorAttachmentFormats[0], render_width, render_height, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  s_prefix.c_str() }; /* G-Buffer Color */
		m_Normal_Output[i].info = { m_ColorAttachmentFormats[1], render_width, render_height, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  s_prefix.c_str() }; /* G-Buffer Normal */
		m_Depth_Output[i].info  = { m_DepthAttachmentFormat,     render_width, render_height, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  s_prefix.c_str()  }; /* G-Buffer Depth */

		m_Albedo_Output[i].CreateImage(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_Normal_Output[i].CreateImage(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_Depth_Output[i].CreateImage(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		m_Albedo_Output[i].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_Normal_Output[i].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_Depth_Output[i].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });

		m_Albedo_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_Normal_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_Depth_Output[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	
	end_temp_cmd_buffer(cmd_buffer);
}


void VulkanModelRenderer::create_buffers()
{
	
}

VulkanModelRenderer::~VulkanModelRenderer()
{
	vkDeviceWaitIdle(context.device);
	m_default_texture.Destroy(context.device);
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_Albedo_Output[i].Destroy(context.device);
		m_Normal_Output[i].Destroy(context.device);
		m_Depth_Output[i].Destroy(context.device);
	}
}
