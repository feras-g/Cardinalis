#include "VulkanPresentRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

VulkanPresentRenderer::VulkanPresentRenderer(const VulkanContext& vkContext, bool useDepth) 
	: VulkanRendererBase(vkContext), bClearDepth(useDepth)
{
}

void VulkanPresentRenderer::Initialize()
{
	if (!CreateColorDepthRenderPass({ false, false, RENDERPASS_LAST }, true, &m_RenderPass))
	{
		LOG_ERROR("Failed to create renderpass.");
		assert(false);
	}
	
	if (!CreateColorDepthFramebuffers(m_RenderPass, *context.swapchain.get()))
	{
		LOG_ERROR("Failed to create framebuffers.");
		assert(false);
	}
}

void VulkanPresentRenderer::PopulateCommandBuffer(VkCommandBuffer cmdBuffer) const
{
	//VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Pass: Present");

	VkRect2D	 screenRect		= { .offset = {0, 0}, .extent = m_FramebufferExtent };

	VkRenderPassBeginInfo beginInfo =
	{
		.sType		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = m_RenderPass,
		.framebuffer = context.swapchain->framebuffers[context.currentBackBuffer],
		.renderArea  = screenRect,
		.clearValueCount = 0,
		.pClearValues = nullptr
	};

	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmdBuffer);
}

VulkanPresentRenderer::~VulkanPresentRenderer()
{
}
