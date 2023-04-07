#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include <vulkan/vulkan.h> 
#include "Rendering/Vulkan/VulkanResources.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <span>

struct Buffer;

// Vertex description
struct VertexData
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct SimpleVertexData
{
	glm::vec3 pos;
	glm::vec2 uv;
};

// Class describing a 3D model in Vulkan
class VulkanModel
{
public:
	VulkanModel() = default;

	void CreateFromFile(const char* filename);
	void CreateFromData(std::span<SimpleVertexData> vertices, std::span<unsigned int> indices);
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
