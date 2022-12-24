#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include <vulkan/vulkan.h> 

// Class describing a 3D model in Vulkan
class VulkanModel
{
public:
	VulkanModel() = default;

	bool CreateFromFile(const char* filename);

	size_t m_VtxBufferSizeInBytes;
	size_t m_IdxBufferSizeInBytes;

	VkBuffer m_StorageBuffer = VK_NULL_HANDLE;

	~VulkanModel() = default;
private:
	// Contains non-interleaved vertex + index data 
	VkDeviceMemory m_StorageBufferMem;
};

#endif // !VULKAN_MODEL_H
