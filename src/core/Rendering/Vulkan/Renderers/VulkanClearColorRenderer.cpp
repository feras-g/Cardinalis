#include "VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"

VulkanClearColorRenderer::VulkanClearColorRenderer(const VulkanContext& vkContext) 
{
	CreateFramebufferAndRenderPass();
}

void VulkanClearColorRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer)
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
		.clearValueCount = 2u,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmdBuffer);
}

bool VulkanClearColorRenderer::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	return false;
}

bool VulkanClearColorRenderer::CreateRenderPass()
{
	m_RenderPassInitInfo = { true, true, context.swapchain->info.colorFormat, context.swapchain->info.depthStencilFormat,  RENDERPASS_FIRST };

	if (!CreateColorDepthRenderPass(m_RenderPassInitInfo, &m_RenderPass))
	{
		LOG_ERROR("Failed to create renderpass.");
		assert(false);
	}

	return true;
}


bool VulkanClearColorRenderer::CreateFramebuffers()
{
	if (!CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), true))
	{
		LOG_ERROR("Failed to create framebuffers.");
		assert(false);
	}

	return true;
}

bool VulkanClearColorRenderer::CreateFramebufferAndRenderPass()
{
	if (m_RenderPass != VK_NULL_HANDLE) 
	{ 
		vkDestroyRenderPass(context.device, m_RenderPass, nullptr); 
	}
	if (m_Framebuffers[0] != VK_NULL_HANDLE)
	{
		for (VkFramebuffer& fb : m_Framebuffers) { vkDestroyFramebuffer(context.device, fb, nullptr); }
	}

	CreateRenderPass();
	CreateFramebuffers();

	return true;
}

VulkanClearColorRenderer::~VulkanClearColorRenderer()
{
}