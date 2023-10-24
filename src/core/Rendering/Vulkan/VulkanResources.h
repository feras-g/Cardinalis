#pragma once

#include "vulkan/vulkan.hpp"

struct Buffer
{
	enum class Type
	{
		NONE, UNIFORM, STORAGE, STAGING, INDIRECT
	} m_type;

	void init(Type buffer_type, size_t size, const char* name);
	void create();
	void destroy();
	bool m_b_initialized = false;

	size_t m_hash;

	operator const VkBuffer& () const { return m_vk_buffer; }
	operator const VkBuffer* () const { return &m_vk_buffer; }

	void* map(VkDevice device, size_t offset, size_t size);
	void unmap(VkDevice device);
	void upload(VkDevice device, const void* data, size_t offset, size_t size);

	VkBuffer_T* m_vk_buffer = VK_NULL_HANDLE;
	VkDeviceMemory_T* m_vk_device_memory = VK_NULL_HANDLE;

	void create_vk_buffer(size_t size);
	void create_vk_buffer_impl(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties);
	const char* m_debug_name;
	size_t m_size_bytes = 0;
	void* data = nullptr;
};

void copy_from_buffer(const Buffer& src, const Buffer& dst, VkDeviceSize size);
