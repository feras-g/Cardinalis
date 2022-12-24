#include "VulkanPresentRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

VulkanPresentRenderer::VulkanPresentRenderer(const VulkanContext& vkContext, bool useDepth) 
	: VulkanRendererBase(vkContext, useDepth)
{
	CreateRenderPass();
	CreateFramebuffers();
}

void VulkanPresentRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Present");

	VkRect2D	 screenRect		= { .offset = {0, 0}, .extent = context.swapchain->info.extent };

	VkRenderPassBeginInfo beginInfo =
	{
		.sType		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = m_RenderPass,
		.framebuffer = m_Framebuffers[currentImageIdx],
		.renderArea  = screenRect,
		.clearValueCount = 0,
		.pClearValues = nullptr
	};

	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmdBuffer);
}

bool VulkanPresentRenderer::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	return false;
}

bool VulkanPresentRenderer::CreateRenderPass()
{
	m_RenderPassInitInfo = { false, false, RENDERPASS_LAST };
	if (!CreateColorDepthRenderPass(m_RenderPassInitInfo, true, &m_RenderPass))
	{
		LOG_ERROR("Failed to create renderpass.");
		assert(false);
	}

	return true;
}

bool VulkanPresentRenderer::CreateFramebuffers()
{
	if (!CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth))
	{
		LOG_ERROR("Failed to create framebuffers.");
		assert(false);
	}

	return true;
}

VulkanPresentRenderer::~VulkanPresentRenderer()
{
}
