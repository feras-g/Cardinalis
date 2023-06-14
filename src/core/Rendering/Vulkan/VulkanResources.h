#pragma once

#include "vulkan/vulkan.hpp"

class Buffer
{
public:
	enum class Type
	{
		NONE, UNIFORM, STORAGE, STAGING
	};

	VkBuffer_T* buffer{ VK_NULL_HANDLE };
	VkDeviceMemory_T* memory{ VK_NULL_HANDLE };
	size_t size_bytes{ 0 };
	void* data{ nullptr };
};

void create_buffer(Buffer::Type type, Buffer& result, size_t size);
void create_buffer_impl(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
void upload_buffer_data(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset);
void copy_buffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
void destroy_buffer(const Buffer& buffer);
