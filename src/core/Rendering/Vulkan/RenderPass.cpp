#include "RenderPass.h"
#include "core/engine/Image.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"

void VulkanRenderPassDynamic::begin(VkCommandBuffer cmd_buffer, glm::vec2 extent, uint32_t viewMask)
{
	VkRect2D area = {};
	area.offset.x = 0;
	area.offset.y = 0;
	area.extent.width  = (uint32_t)extent.x;
	area.extent.height = (uint32_t)extent.y;

	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = area;
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = (uint32_t)color_attachments.size();
	render_info.pColorAttachments = color_attachments.data();
	render_info.pDepthAttachment = has_depth_attachment ? &depth_attachment : VK_NULL_HANDLE;
	render_info.pStencilAttachment = has_stencil_attachment ? &stencil_attachment : VK_NULL_HANDLE;
	render_info.viewMask = viewMask;

	fpCmdBeginRenderingKHR(cmd_buffer, &render_info);
}

void VulkanRenderPassDynamic::end(VkCommandBuffer cmd_buffer) const
{
	fpCmdEndRenderingKHR(cmd_buffer);
}

void  VulkanRenderPassDynamic::reset()
{
	color_attachments.clear();
	has_depth_attachment = false;
	has_stencil_attachment = false;
}

void VulkanRenderPassDynamic::add_attachment(VkImageView view, VkImageLayout layout, VkAttachmentLoadOp loadOp)
{
	assert(view);

	VkRenderingAttachmentInfoKHR attachment_info{};
	attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment_info.imageLayout = layout;
	attachment_info.imageView = view;
	attachment_info.loadOp = loadOp;
	attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	
	if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		attachment_info.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		color_attachments.push_back(attachment_info);
	}
	else if (layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		has_depth_attachment = true;
		attachment_info.clearValue.depthStencil = { 1.0f, 1 };
		depth_attachment = attachment_info;
	}
	else if (layout == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL || VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		has_stencil_attachment = true;
		attachment_info.clearValue.depthStencil = { 1.0f, 1 };
		stencil_attachment = attachment_info;
	}
	else
	{
		LOG_ERROR("Unsupported attachment.");
		assert(false);
	}
}
void VulkanRenderPassDynamic::add_color_attachment(VkImageView view, VkAttachmentLoadOp loadOp)
{
	add_attachment(view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, loadOp);
}

void VulkanRenderPassDynamic::add_depth_attachment(VkImageView view, VkAttachmentLoadOp loadOp)
{
	add_attachment(view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, loadOp);
}

void VulkanRenderPassDynamic::add_stencil_attachment(VkImageView view, VkAttachmentLoadOp loadOp)
{
	add_attachment(view, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, loadOp);
}
