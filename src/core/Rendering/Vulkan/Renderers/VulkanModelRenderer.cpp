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

static PFN_vkCmdBeginRenderingKHR fpCmdBeginRenderingKHR;
static PFN_vkCmdEndRenderingKHR fpCmdEndRenderingKHR;

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename, const char* textureFilename) 
	: VulkanRendererBase(context, true)
{
	if (!m_Model.CreateFromFile(modelFilename))
	{
		LOG_ERROR("Failed to create model from {0}", modelFilename);
	}

	m_Texture.CreateFromFile(textureFilename, "Default Checkerboard Texture", VK_FORMAT_R8G8B8A8_UNORM);
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	m_Texture.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	end_temp_cmd_buffer();
	m_Texture.CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });

	m_Shader.reset(new VulkanShader("Model.vert.spv", "Model.frag.spv"));

	Init();


	fpCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetInstanceProcAddr(context.instance, "vkCmdBeginRenderingKHR"));
	fpCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetInstanceProcAddr(context.instance, "vkCmdEndRenderingKHR"));
	if (!fpCmdBeginRenderingKHR || !fpCmdEndRenderingKHR)
	{
		throw std::runtime_error("Unable to dynamically load vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR");
	}


}

void VulkanModelRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Forward Pass");

	const uint32_t width  = m_ColorAttachments[0].info.width;
	const uint32_t height = m_ColorAttachments[0].info.height;
	
	SetViewportScissor(cmdBuffer, width, height, true);	

	VkClearValue clear_value;
	clear_value.color = { 0.1f, 0.1f, 1.0f, 1.0f};
	clear_value.depthStencil = { 1.0f, 1 };

	VkRect2D render_area = VkRect2D{ VkOffset2D{}, VkExtent2D{width, height} };
	std::array<VkRenderingAttachmentInfoKHR, 2> color_attachment_infos {};
	for (int i = 0; i < m_ColorAttachments.size(); i++)
	{
		color_attachment_infos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment_infos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_infos[i].imageView = m_ColorAttachments[i].view;
		color_attachment_infos[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment_infos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_infos[i].clearValue = clear_value;
	}

	VkRenderingAttachmentInfoKHR depth_stencil_attachment_info {};
	depth_stencil_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_stencil_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_stencil_attachment_info.imageView = m_DepthAttachments[0].view;
	depth_stencil_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_stencil_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_stencil_attachment_info.clearValue = clear_value;

	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = render_area;
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = color_attachment_infos.size();
	render_info.pColorAttachments    = color_attachment_infos.data();
	render_info.pDepthAttachment     = &depth_stencil_attachment_info;
	render_info.pStencilAttachment   = &depth_stencil_attachment_info;

	fpCmdBeginRenderingKHR(cmdBuffer, &render_info);

	draw_scene(currentImageIdx, cmdBuffer);

	fpCmdEndRenderingKHR(cmdBuffer);

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
	if (!CreateAttachments()) return false;
	if (!CreateDescriptorPool(2, 1, 1, &m_DescriptorPool)) return false;
	if (!CreateUniformBuffers(sizeof(UniformData))) return false;
	if (!CreateDescriptorSets(context.device, m_DescriptorSets, &m_DescriptorSetLayout)) return false;
	if (!UpdateDescriptorSets(context.device)) return false;
	if (!CreatePipelineLayout(context.device, m_DescriptorSetLayout, &m_PipelineLayout)) return false;

	GraphicsPipeline::Flags flags = GraphicsPipeline::Flags::ENABLE_DEPTH_STATE;
	if (!GraphicsPipeline::CreateDynamic(*m_Shader.get(), m_ColorAttachmentFormats, m_DepthAttachmentFormat, 
		flags, m_PipelineLayout, &m_GraphicsPipeline, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)) return false;
	
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

bool VulkanModelRenderer::CreateAttachments()
{
	// Create image views for offscreen buffers
	m_ColorAttachments.resize(NUM_FRAMES);
	m_DepthAttachments.resize(NUM_FRAMES);

	m_ColorAttachments[0].info = { m_ColorAttachmentFormats[0], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "G-Buffer Color" };
	m_ColorAttachments[1].info = { m_ColorAttachmentFormats[1], attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED,  "G-Buffer Normal" };

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	for (int i = 0; i < m_ColorAttachments.size(); i++)
	{
		m_ColorAttachments[i].CreateImage(context.device,  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_ColorAttachments[i].CreateView(context.device,  { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_ColorAttachments[i].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	m_DepthAttachments[0].info = { m_DepthAttachmentFormat, attachmentWidth, attachmentHeight, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, "G-Buffer Depth" };
	m_DepthAttachments[0].CreateImage(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	m_DepthAttachments[0].CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
	m_DepthAttachments[0].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	end_temp_cmd_buffer();
	return true;
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
