#pragma once

#include <vulkan/vulkan.hpp> 
#include "Rendering/Vulkan/VulkanResources.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <span>
#include <string>
#include <vector>

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

/* Class describing geometry */
struct VulkanMesh
{
	VulkanMesh() = default;
	VulkanMesh(const char* filename) { create_from_file(filename); }
	void create_from_file(const char* filename);
	void create_from_data(std::span<SimpleVertexData> vertices, std::span<unsigned int> indices);
	size_t m_VtxBufferSizeInBytes;
	size_t m_IdxBufferSizeInBytes;
	size_t m_NumVertices;
	size_t m_NumIndices;

	/* Non-interleaved vertex and index data. Used for vertex pulling. */
	Buffer m_vertex_index_buffer;
	VkDescriptorSet descriptor_set;
};

struct Material
{
};

struct TransformDataUbo
{
	glm::mat4* mvp = nullptr;
};

struct TransformData
{
	glm::vec4 translation;
	glm::vec4 rotation;
	glm::vec4 scale;
};

struct Drawable
{
	VulkanMesh* mesh_handle;
	Drawable(VulkanMesh* mesh);
	void draw(VkCommandBuffer cmd_buffer) const;
};


struct Materials
{
	std::vector<std::string> names;
};