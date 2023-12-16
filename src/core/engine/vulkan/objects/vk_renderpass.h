#pragma once

#include "vulkan/vulkan.hpp"

#include "glm/vec2.hpp"

namespace vk
{
	struct renderpass_dynamic
	{
		void begin(VkCommandBuffer cmd_buffer, glm::vec2 extent, uint32_t viewMask = 0);
		void end(VkCommandBuffer cmd_buffer) const;
		void reset();

		void add_attachment(VkImageView view, VkImageLayout layout, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
		void add_color_attachment(VkImageView view, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
		void add_depth_attachment(VkImageView view, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);
		void add_stencil_attachment(VkImageView view, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR);

		std::vector<VkRenderingAttachmentInfoKHR> color_attachments;

		bool has_depth_attachment = false;
		bool has_stencil_attachment = false;

		VkRenderingAttachmentInfoKHR depth_attachment;
		VkRenderingAttachmentInfoKHR stencil_attachment;
	};
}




