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

bool VulkanRendererBase::CreateColorDepthFramebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain)
{
	VkFramebufferCreateInfo fbInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = 2,
		.layers = 1,
	};

	// One framebuffer per swapchain image
	for (int i = 0; i < swapchain.info.imageCount; i++)
	{
		// 2 attachments per framebuffer : COLOR + DEPTH
		VkImageView attachments[2] = { swapchain.images[i].view, swapchain.depthImages[i].view };

		// COLOR attachment
		fbInfo.pAttachments = attachments;
		fbInfo.width = swapchain.images[i].info.width;
		fbInfo.height = swapchain.images[i].info.height;
		VK_CHECK(vkCreateFramebuffer(context.device, &fbInfo, nullptr, &swapchain.framebuffers[i]));
	}

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
