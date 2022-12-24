#include "VulkanModelRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

#include <glm/mat4x4.hpp>

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

	BeginRenderpass(cmdBuffer, m_RenderPass, m_Framebuffers[currentImageIdx]);
	SetViewportScissor(cmdBuffer);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentImageIdx], 0, nullptr);

	vkCmdDraw(cmdBuffer, m_Model.m_IdxBufferSizeInBytes / sizeof(unsigned int), 1, 0, 0);
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
	m_RenderPassInitInfo = { .clearColor = false, .clearDepth = false };
	return CreateColorDepthRenderPass(m_RenderPassInitInfo, true, &m_RenderPass);
}

bool VulkanModelRenderer::CreateFramebuffers()
{
	return CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth);
}

VulkanModelRenderer::~VulkanModelRenderer()
{
}
