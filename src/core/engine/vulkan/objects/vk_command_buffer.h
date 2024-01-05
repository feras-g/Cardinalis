#pragma once

#include "core/engine/vulkan/vk_common.h"
#include "vk_device.h"

namespace vk
{
	class command_buffer
	{
	public:
		command_buffer() = default;
		operator VkCommandBuffer() const { return m_cmd_buffer; }
		operator VkCommandBuffer&() { return m_cmd_buffer; }

		const VkCommandBuffer* ptr() const { return &m_cmd_buffer; }
		VkResult init(vk::device device, uint32_t queue_family_index);
		VkResult create(vk::device device);
		VkResult begin() const;
		VkResult end() const;
	protected:
		VkCommandBuffer m_cmd_buffer;
		VkCommandPool m_cmd_pool;
	};
}