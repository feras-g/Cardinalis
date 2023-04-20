#pragma once

#include "vulkan/vulkan.hpp"

struct Buffer
{
	VkBuffer_T* buffer{ VK_NULL_HANDLE };
	VkDeviceMemory_T* memory{ VK_NULL_HANDLE };
	size_t size_bytes{ 0 };
	void* data{ nullptr };
};

void create_buffer(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
void create_uniform_buffer(Buffer& result, VkDeviceSize size);
void create_storage_buffer(Buffer& result, VkDeviceSize size);
void create_staging_buffer(Buffer& result, VkDeviceSize size);
void upload_buffer_data(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset);
void copy_buffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
void destroy_buffer(const Buffer& buffer);
