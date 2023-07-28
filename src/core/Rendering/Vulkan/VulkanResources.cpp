#include "VulkanResources.h"
#include "VulkanTools.h"
#include "VulkanRenderInterface.h"
static size_t nb = 0;

void create_buffer(Buffer::Type type, Buffer& result, size_t size)
{
	switch (type)
	{
	case Buffer::Type::UNIFORM:
		create_buffer_impl(result, size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	case Buffer::Type::STORAGE:
		create_buffer_impl(result, size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	case Buffer::Type::STAGING:
		create_buffer_impl(result, size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		break;
	default:
		LOG_ERROR("Unknown buffer type.");
		break;
	}
}

void create_buffer_impl(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties)
{
	LOG_INFO("Create buffer {}", nb++);
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

	VK_CHECK(vkCreateBuffer(context.device, &info, nullptr, &result.buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.device, result.buffer, &memRequirements);
	
	result.size_bytes = memRequirements.size;

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(context.physical_device, memRequirements.memoryTypeBits, memProperties)
	};
	VK_CHECK(vkAllocateMemory(context.device, &allocInfo, nullptr, &result.memory));

	VK_CHECK(vkBindBufferMemory(context.device, result.buffer, result.memory, 0));
}

void upload_buffer_data(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset)	
{
	void* pMappedData = nullptr;
	VK_CHECK(vkMapMemory(context.device, buffer.memory, offset, size, 0, (void**)&pMappedData));
	memcpy(pMappedData, data, size);
	vkUnmapMemory(context.device, buffer.memory);
}

void copy_buffer(const Buffer& src, const Buffer& dst, VkDeviceSize size)
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
	VkBufferCopy bufferRegion = { .srcOffset = 0, .dstOffset = 0, .size = size };
	vkCmdCopyBuffer(cmd_buffer, src.buffer, dst.buffer, 1, &bufferRegion);
	end_temp_cmd_buffer(cmd_buffer);
}

void destroy_buffer(const Buffer& buffer)
{
	if (buffer.buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(context.device, buffer.buffer, nullptr);
		vkFreeMemory(context.device, buffer.memory, nullptr);
		LOG_INFO("Destroy buffer {}", nb--);
	}
}