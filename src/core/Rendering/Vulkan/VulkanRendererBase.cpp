#include "VulkanRendererBase.h"

VulkanRendererBase::VulkanRendererBase(const VulkanContext& vkContext, bool useDepth)
	: bUseDepth(useDepth)
{

}

VulkanRendererBase::~VulkanRendererBase()
{
	//for (size_t i = 0; i < NUM_FRAMES; i++)
	//{
	//	vkDestroyBuffer(context.device, m_UniformBuffers[i], nullptr);
	//}

	//for (size_t i = 0; i < NUM_FRAMES; i++)
	//{
	//	vkFreeMemory(context.device, m_UniformBuffersMemory[i], nullptr);
	//}

	//vkDestroyDescriptorSetLayout(context.device, m_DescriptorSetLayout, nullptr);
	//vkDestroyDescriptorPool(context.device, m_DescriptorPool, nullptr);
	//vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	//vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
	//vkDestroyPipeline(context.device, m_GraphicsPipeline, nullptr);
}

bool VulkanRendererBase::RecreateFramebuffersRenderPass()
{
	for (int i=0; i < m_Framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(context.device, m_Framebuffers[i], nullptr);
	}
	
	vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	CreateColorDepthRenderPass(m_RenderPassInitInfo, bUseDepth, &m_RenderPass);
	CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth);
	
	return true;
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
