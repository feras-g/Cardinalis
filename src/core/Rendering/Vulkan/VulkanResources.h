#pragma once

#include "vulkan/vulkan.hpp"

struct Buffer
{
	VkBuffer_T* buffer { VK_NULL_HANDLE };
	VkDeviceMemory_T* memory { VK_NULL_HANDLE };
	size_t size_bytes { 0 };
	void* data { nullptr };
};

void CreateBuffer(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
void create_uniform_buffer(Buffer& result, VkDeviceSize size);
void create_storage_buffer(Buffer& result, VkDeviceSize size);
void CreateStagingBuffer(Buffer& result, VkDeviceSize size);
void UploadBufferData(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset);
void CopyBuffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
void DestroyBuffer(const Buffer& buffer);
