#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

#include <glm/mat4x4.hpp>

const uint32_t attachmentWidth  = 800;
const uint32_t attachmentHeight = 800;

struct UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
} ModelUniformData;

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename, const char* textureFilename) 
	: VulkanRendererBase(context, true)
{
	if (!m_Model.CreateFromFile(modelFilename))
	{
		LOG_ERROR("Failed to create model from {0}", modelFilename);
	}

	m_Texture.CreateFromFile(textureFilename, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	m_Texture.CreateImageView(context.device, { .format = VK_FORMAT_R8G8B8A8_UNORM, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT });

	m_Shader.reset(new VulkanShader("Model.vert.spv", "Model.frag.spv"));

	CreatePipeline();
}

void VulkanModelRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Forward Pass");

	const uint32_t width   = m_ColorAttachments[0].info.width;
	const uint32_t height = m_ColorAttachments[0].info.height;
	
	SetViewportScissor(cmdBuffer, { 0,0, (float)width, (float)height }, {0,0, width, height });

	VkClearValue clearValues[2] = { {.color = {0.01f, 0.01f, 1.0f, 1.0f}}, {.depthStencil = {1.0f, 1}} };

	BeginRenderpass(cmdBuffer, m_RenderPass, m_Framebuffers[currentImageIdx], {0, 0, width, height}, clearValues, 2);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentImageIdx], 0, nullptr);

	vkCmdDraw(cmdBuffer, m_Model.m_NumVertices, 1, 0, 0);
	EndRenderPass(cmdBuffer);
}

bool VulkanModelRenderer::CreatePipeline()
{
	if (!CreateRenderPass())   return false;
	if (!CreateFramebuffers()) return false;
	if (!CreateDescriptorPool(2, 1, 1, &m_DescriptorPool)) return false;
	if (!CreateUniformBuffers(sizeof(UniformData))) return false;
	if (!CreateDescriptorSets(context.device, m_DescriptorSets, &m_DescriptorSetLayout)) return false;
	if (!UpdateDescriptorSets(context.device)) return false;
	if (!CreatePipelineLayout(context.device, m_DescriptorSetLayout, &m_PipelineLayout)) return false;
	if (!CreateGraphicsPipeline(*m_Shader.get(), false, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_RenderPass, m_PipelineLayout, &m_GraphicsPipeline)) return false;

	return true;
}

bool VulkanModelRenderer::UpdateBuffers(size_t currentImage, glm::mat4 model, glm::mat4 view, glm::mat4 proj)
{
	ModelUniformData.model = model;
	ModelUniformData.view  = view;
	ModelUniformData.proj  = proj;

	UploadBufferData(m_UniformBuffersMemory[currentImage], 0, &ModelUniformData, sizeof(UniformData));

	return true;
}


bool VulkanModelRenderer::UpdateDescriptorSets(VkDevice device)
{
	// Mesh data is not modified between 2 frames
	VkDescriptorBufferInfo sboInfo0 = { .buffer = m_Model.m_StorageBuffer, .offset = 0, .range = m_Model.m_VtxBufferSizeInBytes };
	VkDescriptorBufferInfo sboInfo1 = { .buffer = m_Model.m_StorageBuffer, .offset = m_Model.m_VtxBufferSizeInBytes, .range = m_Model.m_IdxBufferSizeInBytes };
	VkDescriptorImageInfo imageInfo0 = { .sampler = s_SamplerRepeatLinear, .imageView = m_Texture.view, .imageLayout = m_Texture.info.layout };

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		VkDescriptorBufferInfo uboInfo0  = { .buffer = m_UniformBuffers[i], .offset = 0, .range = sizeof(UniformData) };

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

bool VulkanModelRenderer::CreateRenderPass()
{
	m_RenderPassInitInfo = { true, true, m_ColorFormat, m_DepthStencilFormat, RENDERPASS_INTERMEDIATE_OFFSCREEN };

	return CreateColorDepthRenderPass(m_RenderPassInitInfo, true, &m_RenderPass);
}

bool VulkanModelRenderer::CreateFramebuffers()
{
	// Create offscreen framebuffers 
	// Used to integrate them in a UI viewport
	StartInstantUseCmdBuffer();
	m_ColorAttachments.resize(NUM_FRAMES);
	m_DepthStencilAttachments.resize(NUM_FRAMES);
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_ColorAttachments[i].CreateImage(context.device, { .width = attachmentWidth, .height = attachmentHeight, .format = m_ColorFormat, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT });
		m_ColorAttachments[i].CreateImageView(context.device, { .format = m_ColorFormat, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT });
		m_ColorAttachments[i].Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		m_DepthStencilAttachments[i].CreateImage(context.device, { .width = attachmentWidth, .height = attachmentHeight, .format = m_DepthStencilFormat, .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT });
		m_DepthStencilAttachments[i].CreateImageView(context.device, { .format = m_DepthStencilFormat, .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT });
		m_DepthStencilAttachments[i].Transition(context.mainCmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	EndInstantUseCmdBuffer();

	return CreateColorDepthFramebuffers(m_RenderPass, m_ColorAttachments.data(), m_DepthStencilAttachments.data(), m_Framebuffers.data(), bUseDepth);
}

VulkanModelRenderer::~VulkanModelRenderer()
{
}
