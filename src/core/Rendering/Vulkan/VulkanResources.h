#ifndef VULKAN_RESOURCES_H
#define VULKAN_RESOURCES_H

#include <vulkan/vulkan.h>

struct Buffer
{
	VkBuffer buffer		  = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	void* data = nullptr;
	size_t size = 0;
};

void CreateBuffer(Buffer& result, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
void CreateUniformBuffer(Buffer& result, VkDeviceSize size);
void CreateStorageBuffer(Buffer& result, VkDeviceSize size);
void CreateStagingBuffer(Buffer& result, VkDeviceSize size);
void UploadBufferData(Buffer& buffer, const void* data, const size_t size, VkDeviceSize offset);
void CopyBuffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
void DestroyBuffer(const Buffer& buffer);

#endif // VULKAN_RESOURCES_H