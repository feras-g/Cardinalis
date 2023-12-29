#include "vk_buffer.h"
#include "core/engine/Image.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/engine/vulkan/objects/vk_debug_marker.hpp"
#include "core/rendering/vulkan/VkResourceManager.h"

namespace vk
{
	void vk::buffer::init(type buffer_type, size_t size, const char* name)
	{
		m_type = buffer_type;
		m_size_bytes = size;
		m_debug_name = name;
		m_b_initialized = true;
	}

	void vk::buffer::create()
	{
		assert(m_b_initialized);
		create_vk_buffer(m_size_bytes);
		set_object_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_vk_buffer, m_debug_name);
	}

	void vk::buffer::destroy()
	{
		VkResourceManager::get_instance(ctx.device)->destroy_buffer(m_hash);
	}

	void* vk::buffer::map(VkDevice device, size_t offset, size_t size)
	{
		assert(m_vk_device_memory);
		void* p_data = nullptr;
		vkMapMemory(device, m_vk_device_memory, offset, size, 0, &p_data);
		return p_data;
	}

	void vk::buffer::unmap(VkDevice device)
	{
		assert(m_vk_device_memory);
		vkUnmapMemory(device, m_vk_device_memory);
	}

	void vk::buffer::upload(VkDevice device, const void* data, size_t offset, size_t size)
	{
		void* p_data = map(device, offset, size);
		memcpy(p_data, data, size);
		unmap(device);
	}

	void vk::buffer::create_vk_buffer(size_t size)
	{
		switch (m_type)
		{
		case vk::buffer::type::UNIFORM:
			create_vk_buffer_impl(size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		case vk::buffer::type::STORAGE:
			create_vk_buffer_impl(size,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		case vk::buffer::type::STAGING:
			create_vk_buffer_impl(size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		case vk::buffer::type::INDIRECT:
			create_vk_buffer_impl(size,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			break;
		default:
			LOG_ERROR("Unknown buffer type.");
			assert(false);
			break;
		}
	}

	void vk::buffer::create_vk_buffer_impl(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties)
	{

		VkBufferCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.flags = 0,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr
		};

		VK_CHECK(vkCreateBuffer(ctx.device, &info, nullptr, &m_vk_buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(ctx.device, m_vk_buffer, &memRequirements);
		m_size_bytes = memRequirements.size;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = ctx.device.find_memory_type(memRequirements.memoryTypeBits, memProperties);
		VK_CHECK(vkAllocateMemory(ctx.device, &allocInfo, nullptr, &m_vk_device_memory));

		VK_CHECK(vkBindBufferMemory(ctx.device, m_vk_buffer, m_vk_device_memory, 0));

		/* Add to manager */
		m_hash = VkResourceManager::get_instance(ctx.device)->add_buffer(m_vk_buffer, m_vk_device_memory);
	}
}
void copy_from_buffer(const vk::buffer& src, const vk::buffer& dst, VkDeviceSize size)
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	VkBufferCopy bufferRegion = { .srcOffset = 0, .dstOffset = 0, .size = size };
	vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &bufferRegion);
	end_temp_cmd_buffer(cmd_buffer);
}
