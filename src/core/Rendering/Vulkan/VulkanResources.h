#ifndef VULKAN_RESOURCES_H
#define VULKAN_RESOURCES_H

#include "vulkan/vulkan.h"

struct Buffer
{
	VkBuffer_T* buffer { VK_NULL_HANDLE };
	VkDeviceMemory_T* memory { VK_NULL_HANDLE };
	size_t sizeInBytes { 0 };
	void* data { nullptr };
};

void CreateBuffer(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
void CreateUniformBuffer(Buffer& result, VkDeviceSize size);
void CreateStorageBuffer(Buffer& result, VkDeviceSize size);
void CreateStagingBuffer(Buffer& result, VkDeviceSize size);
void UploadBufferData(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset);
void CopyBuffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
void DestroyBuffer(const Buffer& buffer);

#endif // VULKAN_RESOURCES_H