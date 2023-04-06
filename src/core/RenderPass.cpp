#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanTools.h"
#include "vulkan/vulkan.hpp"

using namespace vk;

void DynamicRenderPass::begin(VkCommandBuffer cmd_buffer, VkRect2D render_area)
{
	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = render_area;
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = color_attachments.size();
	render_info.pColorAttachments = color_attachments.data();
	render_info.pDepthAttachment = &depth_attachment;
	render_info.pStencilAttachment = &stencil_attachment;

	vkCmdBeginRenderingKHR(cmd_buffer, &render_info);
}

void DynamicRenderPass::end(VkCommandBuffer cmd_buffer) const
{
	vkCmdEndRenderingKHR(cmd_buffer);
}

void DynamicRenderPass::add_attachment(VkImageView view, VkImageLayout layout)
{
	VkRenderingAttachmentInfoKHR attachment_info{};
	attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment_info.imageLayout = layout;
	attachment_info.imageView = view;
	attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	if (!!(layout & VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
	{
		attachment_info.clearValue.color = { 1.0f, 0.0f, 1.0f, 1.0f };
		color_attachments.push_back(attachment_info);
	}
	else if (!!(layout & VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
	{
		attachment_info.clearValue.depthStencil = { 1.0f, 1 };
		depth_attachment = attachment_info;
	}
	else
	{
		LOG_ERROR("Unsupported attachment.");
		assert(false);
	}
}
void DynamicRenderPass::add_color_attachment(VkImageView view)
{
	add_attachment(view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void DynamicRenderPass::add_depth_attachment(VkImageView view)
{
	add_attachment(view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
}

void DynamicRenderPass::add_stencil_attachment(VkImageView view)
{
	add_attachment(view, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);
}
