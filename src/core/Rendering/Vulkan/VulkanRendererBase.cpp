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

void VulkanRendererBase::BeginRenderPass(VkCommandBuffer cmdBuffer)
{
	VkRenderPassBeginInfo renderPassBegin =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_RenderPass,
		.framebuffer = context.swapchain->framebuffers[context.currentBackBuffer],
		.renderArea = {.offset = { 0, 0 }, .extent = m_FramebufferExtent }
	};

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[context.currentBackBuffer], 0, nullptr);
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
