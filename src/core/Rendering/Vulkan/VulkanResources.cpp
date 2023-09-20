#include "VulkanResources.h"
#include "VulkanTools.h"
#include "VulkanRenderInterface.h"
#include "VulkanDebugUtils.h"
#include "core/rendering/vulkan/VkResourceManager.h"


void Buffer::init(Type buffer_type, size_t size, const char* name)
{
	type = buffer_type; 
	m_debug_name = name;
	create_vk_buffer(size);
	set_object_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk_buffer, m_debug_name);
}

void Buffer::destroy()
{
	VkResourceManager::get_instance(context.device)->destroy_buffer(hash);
}

void* Buffer::map(VkDevice device, size_t offset, size_t size)
{
	void* p_data = nullptr;
	vkMapMemory(device, vk_device_memory, offset, size, 0, &p_data);
	return p_data;
}

void Buffer::unmap(VkDevice device)
{
	vkUnmapMemory(device, vk_device_memory);
}

void Buffer::upload(VkDevice device, const void* data, size_t offset, size_t size)
{
	void* p_data = map(device, offset, size);
	memcpy(p_data, data, size);
	unmap(device);
}

void Buffer::create_vk_buffer(size_t size)
{
	switch (type)
	{
	case Buffer::Type::UNIFORM:
		create_vk_buffer_impl(size, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	case Buffer::Type::STORAGE:
		create_vk_buffer_impl(size, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	case Buffer::Type::STAGING:
		create_vk_buffer_impl(size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	default:
		LOG_ERROR("Unknown buffer type.");
		assert(false);
		break;
	}
}

void Buffer::create_vk_buffer_impl(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties)
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

	VK_CHECK(vkCreateBuffer(context.device, &info, nullptr, &vk_buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.device, vk_buffer, &memRequirements);
	size_bytes = memRequirements.size;

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = EngineUtils::FindMemoryType(context.physical_device, memRequirements.memoryTypeBits, memProperties);
	VK_CHECK(vkAllocateMemory(context.device, &allocInfo, nullptr, &vk_device_memory));

	VK_CHECK(vkBindBufferMemory(context.device, vk_buffer, vk_device_memory, 0));

	/* Add to manager */
	hash = VkResourceManager::get_instance(context.device)->add_buffer(vk_buffer, vk_device_memory);
}

void copy(const Buffer& src, const Buffer& dst, VkDeviceSize size)
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	VkBufferCopy bufferRegion = { .srcOffset = 0, .dstOffset = 0, .size = size };
	vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &bufferRegion);
	end_temp_cmd_buffer(cmd_buffer);
}
