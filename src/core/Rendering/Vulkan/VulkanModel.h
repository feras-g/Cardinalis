#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include <vulkan/vulkan.h> 
#include "Rendering/Vulkan/VulkanResources.h"

struct Buffer;

// Class describing a 3D model in Vulkan
class VulkanModel
{
public:
	VulkanModel() = default;

	bool CreateFromFile(const char* filename);
	void draw_indexed(VkCommandBuffer cbuf);
	size_t m_VtxBufferSizeInBytes;
	size_t m_IdxBufferSizeInBytes;
	size_t m_NumVertices;
	size_t m_NumIndices;

	// Contains non-interleaved vertex + index data 
	Buffer m_StorageBuffer;

	~VulkanModel() = default;
private:
	
};

#endif // !VULKAN_MODEL_H
