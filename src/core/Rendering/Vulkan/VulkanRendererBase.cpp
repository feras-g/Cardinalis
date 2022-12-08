#include "VulkanRendererBase.h"

VulkanRendererBase::VulkanRendererBase(const VulkanContext& vkContext)
	: m_FramebufferExtent(vkContext.swapchain->info.extent)
{
}

VulkanRendererBase::~VulkanRendererBase()
{
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		vkDestroyBuffer(context.device, m_UniformBuffers[i], nullptr);
	}

	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		vkFreeMemory(context.device, m_UniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorSetLayout(context.device, m_DescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(context.device, m_DescriptorPool, nullptr);
	vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(context.device, m_GraphicsPipeline, nullptr);
}

bool VulkanRendererBase::CreateUniformBuffers(size_t uniformDataSize)
{
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		if (!CreateUniformBuffer(uniformDataSize, m_UniformBuffers[i], m_UniformBuffersMemory[i]))
		{
			LOG_ERROR("Failed to create uniform buffers.");
			return false;
		}
	}

	return true;
}
