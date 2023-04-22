#pragma once

#include <vulkan/vulkan.hpp> 
#include "Rendering/Vulkan/VulkanResources.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/hash.hpp>

#include <span>
#include <string>
#include <vector>
#include <unordered_map>

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

struct Primitive
{
	uint32_t first_index;
	uint32_t index_count;
	glm::mat4 mat_model;
};

struct GeometryData
{
	std::vector<VertexData> vertices{};
	std::vector<unsigned int> indices{};
	std::vector<Primitive>  primitives{};

	glm::mat4 world_mat;
};

/* Class describing geometry */
struct VulkanMesh
{
	VulkanMesh() = default;
	VulkanMesh(const char* filename) { create_from_file(filename); }
	void create_from_file(const char* filename);
	void create_from_file_gltf(const char* filename);
	void create_from_data(std::span<SimpleVertexData> vertices, std::span<unsigned int> indices);
	size_t m_VtxBufferSizeInBytes;
	size_t m_IdxBufferSizeInBytes;
	size_t m_NumVertices;
	size_t m_NumIndices;

	/* Non-interleaved vertex and index data. Used for vertex pulling. */
	Buffer m_vertex_index_buffer;
	VkDescriptorSet descriptor_set;

	GeometryData geometry_data{};
};

struct Material
{
};

struct TransformDataUbo
{
	glm::mat4 mvp;
	glm::mat4 model;
};

struct TransformData
{
	glm::vec4 translation;
	glm::vec4 rotation;
	glm::vec4 scale;
};

struct Drawable
{
	uint32_t base_vertex = 0;

	VulkanMesh* mesh_handle;
	Drawable(VulkanMesh* mesh);
	void draw(VkCommandBuffer cmd_buffer) const;
	void draw_primitives(VkCommandBuffer cmd_buffer) const;
};

struct Materials
{
	std::vector<std::string> names;
};




