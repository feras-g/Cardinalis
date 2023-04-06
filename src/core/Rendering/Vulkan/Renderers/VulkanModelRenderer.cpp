#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"

#include <glm/mat4x4.hpp>

/** Size in pixels of the offscreen buffers */
const uint32_t attachmentWidth  = 1024;
const uint32_t attachmentHeight = 1024;

struct UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
} ModelUniformData;

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename) 
	: VulkanRendererBase(context, true)
{
	if (!m_Model.CreateFromFile(modelFilename))
	{
		LOG_ERROR("Failed to create model from {0}", modelFilename);
	}

	m_Shader.reset(new VulkanShader("Model.vert.spv", "Model.frag.spv"));

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
		/* Each frame has 1 color/normal/depth g-buffer */
		if (i == 0)
		{
			m_dyn_renderpass[i].add_color_attachment(m_ColorAttachments[0].view);
			m_dyn_renderpass[i].add_color_attachment(m_ColorAttachments[1].view);
		}
		else
		{
			m_dyn_renderpass[i].add_color_attachment(m_ColorAttachments[2].view);
			m_dyn_renderpass[i].add_color_attachment(m_ColorAttachments[3].view);
		}

		m_dyn_renderpass[i].add_depth_attachment(m_DepthAttachments[i].view);
	}

	GraphicsPipeline::Flags ppl_flags = GraphicsPipeline::Flags::ENABLE_DEPTH_STATE;
	GraphicsPipeline::CreateDynamic(*m_Shader.get(), m_ColorAttachmentFormats, m_DepthAttachmentFormat, ppl_flags, m_PipelineLayout, &m_GraphicsPipeline, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
}

void VulkanModelRenderer::render(size_t currentImageIdx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Pass");

	/* Transition */
	for (int i = 0; i < m_ColorAttachments.size(); i++)
	{
		m_ColorAttachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	VkRect2D render_area{ .offset {}, .extent { m_ColorAttachments[0].info.width, m_ColorAttachments[0].info.height } };
	m_dyn_renderpass[currentImageIdx].begin(cmd_buffer, render_area);
	draw_scene(currentImageIdx, cmd_buffer);
	m_dyn_renderpass[currentImageIdx].end(cmd_buffer);

	/* Transition */
	for (int i = 0; i < m_ColorAttachments.size(); i++)
	{
		m_ColorAttachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void VulkanModelRenderer::draw_scene(size_t currentImageIdx, VkCommandBuffer cmdBuffer)
{
	uint32_t width = context.swapchain->info.extent.width;
	uint32_t height = context.swapchain->info.extent.height;
	SetViewportScissor(cmdBuffer, width, height);

	// Descriptor sets

	// Graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentImageIdx], 0, nullptr);
	vkCmdDraw(cmdBuffer, m_Model.m_NumVertices, 1, 0, 0);
	//m_Model.draw_indexed(cmdBuffer);
}

bool VulkanModelRenderer::Init()
{

	
	return true;
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


	/* Create G-Buffers for Color, Normal, Depth
	   Each frame has its own G-Buffer to work on */
	m_ColorAttachments.resize(NUM_FRAMES * 2);
	m_DepthAttachments.resize(NUM_FRAMES);

	/* G-Buffer Color */
	m_ColorAttachments[0].info = { m_ColorAttachmentFormats[0], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "Frame #0: G-Buffer Color" };
	m_ColorAttachments[1].info = { m_ColorAttachmentFormats[1], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "Frame #0: G-Buffer Normal" };

	/* G-Buffer Normal */
	m_ColorAttachments[2].info = { m_ColorAttachmentFormats[0], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "Frame #1: G-Buffer Color" };
	m_ColorAttachments[3].info = { m_ColorAttachmentFormats[1], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "Frame #1: G-Buffer Normal" };

	for (int i = 0; i < m_ColorAttachments.size(); i++)
	{
		m_ColorAttachments[i].CreateImage(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_ColorAttachments[i].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_ColorAttachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	/* G-Buffer Depth */
	m_DepthAttachments[0].info = m_DepthAttachments[1].info = { m_DepthAttachmentFormat, attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, "" };
	m_DepthAttachments[0].info.debugName = "Frame #0: G-Buffer Depth";
	m_DepthAttachments[1].info.debugName = "Frame #0: G-Buffer Normal";
	for (int i = 0; i < m_DepthAttachments.size(); i++)
	{
		m_DepthAttachments[i].CreateImage(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_DepthAttachments[i].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_DepthAttachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	
	end_temp_cmd_buffer(cmd_buffer);
}

VulkanModelRenderer::~VulkanModelRenderer()
{
	m_Texture.Destroy(context.device);
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_ColorAttachments[i].Destroy(context.device);
		m_DepthAttachments[i].Destroy(context.device);
	}
}
