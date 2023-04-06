#pragma once

#include "vulkan/vulkan.hpp"

/* Dynamic render pass description */

namespace vk
{
	struct DynamicRenderPass
	{
		void begin(VkCommandBuffer cmd_buffer, VkRect2D render_area);
		void end(VkCommandBuffer cmd_buffer) const;

		void add_attachment(VkImageView view, VkImageLayout layout);
		void add_color_attachment(VkImageView view);
		void add_depth_attachment(VkImageView view);
		void add_stencil_attachment(VkImageView view);

		std::vector<VkRenderingAttachmentInfoKHR> color_attachments;

		bool has_depth_attachment = false;
		bool has_stencil_attachment = false;

		VkRenderingAttachmentInfoKHR depth_attachment;
		VkRenderingAttachmentInfoKHR stencil_attachment;
	};
}


