#include "vk_command_buffer.h"

namespace vk
{
	VkResult command_buffer::init(vk::device device, uint32_t queue_family_index)
	{
		VkCommandPoolCreateInfo cmd_pool_create_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family_index,
		};

		return vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &m_cmd_pool);
	}

	VkResult vk::command_buffer::create(vk::device device)
	{
		VkCommandBufferAllocateInfo allocate_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = VK_NULL_HANDLE,
			.commandPool = m_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1u,
		};

		return vkAllocateCommandBuffers(device, &allocate_info, &m_cmd_buffer);
	}


	VkResult vk::command_buffer::begin() const
	{
		VkCommandBufferBeginInfo begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		return vkBeginCommandBuffer(m_cmd_buffer, &begin_info);
	}

	VkResult vk::command_buffer::end() const
	{
		return vkEndCommandBuffer(m_cmd_buffer);
	}
}
