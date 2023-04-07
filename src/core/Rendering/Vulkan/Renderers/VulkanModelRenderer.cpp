#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

#include <glm/mat4x4.hpp>

/** Size in pixels of the offscreen buffers */
const uint32_t render_width  = 1024;
const uint32_t render_height = 1024;

struct UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
} ModelUniformData;

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename) 
	: VulkanRendererBase(context, true)
{
	m_Model.CreateFromFile(modelFilename);

	m_Shader.reset(new VulkanShader("Deferred.vert.spv", "Deferred.frag.spv"));

	/* Render objects creation */
	create_attachments();
	CreateDescriptorPool(2, 1, 1, &m_DescriptorPool);
	CreateUniformBuffers(sizeof(UniformData));
	CreateDescriptorSets(context.device, m_DescriptorSets, &m_DescriptorSetLayout);
	UpdateDescriptorSets(context.device);
	CreatePipelineLayout(context.device, m_DescriptorSetLayout, &m_PipelineLayout);

	/* Dynamic renderpass setup */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].add_color_attachment(m_Albedo_Output[i].view);
		m_dyn_renderpass[i].add_color_attachment(m_Normal_Output[i].view);
		m_dyn_renderpass[i].add_depth_attachment(m_Depth_Output[i].view);
	}

	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::Flags::ENABLE_DEPTH_STATE;
	GraphicsPipeline::CreateDynamic(*m_Shader.get(), m_ColorAttachmentFormats, m_DepthAttachmentFormat, ppl_flags, m_PipelineLayout, &m_GraphicsPipeline, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void VulkanModelRenderer::render(size_t currentImageIdx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Pass");

	/* Transition */
	m_Albedo_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_Normal_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	m_Depth_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { render_width , render_height } };
	m_dyn_renderpass[currentImageIdx].begin(cmd_buffer, render_area);
	draw_scene(currentImageIdx, cmd_buffer);
	m_dyn_renderpass[currentImageIdx].end(cmd_buffer);

	/* Transition */
	m_Albedo_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Normal_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_Depth_Output[currentImageIdx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanModelRenderer::draw_scene(size_t currentImageIdx, VkCommandBuffer cmdBuffer)
{
	SetViewportScissor(cmdBuffer, render_width, render_height, true);

	// Graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentImageIdx], 0, nullptr);
	m_Model.draw_indexed(cmdBuffer);
}

bool VulkanModelRenderer::UpdateBuffers(size_t currentImage, glm::mat4 model, glm::mat4 view, glm::mat4 proj)
{
	ModelUniformData.model = model;
	ModelUniformData.view  = view;
	ModelUniformData.proj  = proj;

	UploadBufferData(m_UniformBuffers[currentImage], &ModelUniformData, sizeof(UniformData), 0);

	return true;
}


bool VulkanModelRenderer::UpdateDescriptorSets(VkDevice device)
{
	// Mesh data is not modified between 2 frames
	VkDescriptorBufferInfo sboInfo0 = { .buffer = m_Model.m_StorageBuffer.buffer, .offset = 0, .range = m_Model.m_VtxBufferSizeInBytes };
	VkDescriptorBufferInfo sboInfo1 = { .buffer = m_Model.m_StorageBuffer.buffer, .offset = m_Model.m_VtxBufferSizeInBytes, .range = m_Model.m_IdxBufferSizeInBytes };
	VkDescriptorImageInfo imageInfo0 = { .sampler = s_SamplerRepeatLinear, .imageView = m_Texture.view, .imageLayout = m_Texture.info.imageLayout };

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		VkDescriptorBufferInfo uboInfo0  = { .buffer = m_UniformBuffers[i].buffer, .offset = 0, .range = sizeof(UniformData) };

		std::array<VkWriteDescriptorSet, 4> descriptorWrites =
		{
			BufferWriteDescriptorSet(m_DescriptorSets[i], 0, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 1, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 2, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			ImageWriteDescriptorSet(m_DescriptorSets[i],  3,  &imageInfo0)
		};

		vkUpdateDescriptorSets(device, (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

void VulkanModelRenderer::create_attachments()
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	/* Input attachments */
	m_Texture.CreateFromFile("../../../data/textures/default.png", "Default Checkerboard Texture", VK_FORMAT_R8G8B8A8_UNORM);
	m_Texture.CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
	m_Texture.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

VulkanModelRenderer::~VulkanModelRenderer()
{
	m_Texture.Destroy(context.device);
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_Albedo_Output[i].Destroy(context.device);
		m_Normal_Output[i].Destroy(context.device);
		m_Depth_Output[i].Destroy(context.device);
	}
}
