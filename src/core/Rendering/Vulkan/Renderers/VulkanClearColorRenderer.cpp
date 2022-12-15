#include "VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

VulkanClearColorRenderer::VulkanClearColorRenderer(const VulkanContext& vkContext, bool useDepth) 
	: VulkanRendererBase(vkContext, useDepth)
{
	CreateRenderPass();
	CreateFramebuffers();
}

void VulkanClearColorRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) const
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Clear Color");

	// Clear color/depth
	VkClearValue clearValues[2] = { {.color = {0.01f, 0.01f, 1.0f, 1.0f}}, {.depthStencil = {1.0f, 1}} };
	VkRect2D	 screenRect		= { .offset = {0, 0}, .extent = context.swapchain->info.extent };

	VkRenderPassBeginInfo beginInfo =
	{
		.sType		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = m_RenderPass,
		.framebuffer = m_Framebuffers[currentImageIdx],
		.renderArea  = screenRect,
		.clearValueCount = bUseDepth ? 2u : 1u,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmdBuffer);
}

bool VulkanClearColorRenderer::CreateRenderPass()
{
	m_RenderPassInitInfo = { true, bUseDepth, RENDERPASS_FIRST };

	if (!CreateColorDepthRenderPass(m_RenderPassInitInfo, true, &m_RenderPass))
	{
		LOG_ERROR("Failed to create renderpass.");
		assert(false);
	}

	return true;
}

bool VulkanClearColorRenderer::CreateFramebuffers()
{
	if (!CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth))
	{
		LOG_ERROR("Failed to create framebuffers.");
		assert(false);
	}

	return true;
}

VulkanClearColorRenderer::~VulkanClearColorRenderer()
{
}
