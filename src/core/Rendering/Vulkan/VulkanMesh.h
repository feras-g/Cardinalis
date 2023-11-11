#pragma once

#include <vulkan/vulkan.hpp> 
#include "core/rendering/vulkan/VulkanResources.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/hash.hpp>

#include <span>
#include <string>
#include <vector>
#include <unordered_map>

// Vertex description
struct VertexData
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 uv;
};

struct SimpleVertexData
{
	glm::vec3 pos;
	glm::vec2 uv;
};

struct Primitive
{
	uint32_t first_vertex;
	uint32_t vertex_count;
	glm::mat4 model = glm::identity<glm::mat4>();
	int material_id = 0;
};

struct GeometryData
{
	std::vector<VertexData> vertices{};
	std::vector<unsigned int> indices{};
	std::vector<Primitive>  primitives{};
	std::vector<glm::vec4> punctual_lights_positions;
	std::vector<glm::vec4> punctual_lights_colors;


	glm::mat4 world_mat = glm::identity<glm::mat4>();
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::vec4 bbox_min_WS {};
	glm::vec4 bbox_max_WS {};
};

enum class TextureType
{
	NONE = 0,
	ALBEDO_MAP = 1,
	NORMAL_MAP = 2,
	METALLIC_ROUGHNESS_MAP = 3,
	EMISSIVE_MAP = 4
};

struct Node;

/* Class describing geometry */
struct VulkanMesh
{
	void create_from_file(const std::string& filename);
	void create_from_file_gltf(const std::string& filename);
	void create_from_data(std::span<VertexData> vertices, std::span<unsigned int> indices);
	void destroy();

	size_t m_vertex_buf_size_bytes;
	size_t m_index_buf_size_bytes;
	size_t m_num_vertices;
	size_t m_num_indices;

	size_t material_id = 0;

	glm::mat4 model;

	/* Non-interleaved vertex and index data. Used for vertex pulling. */
	Buffer m_vertex_index_buffer;

	GeometryData geometry_data;
	std::vector<Node*> nodes;
};


// A node represents an object in the glTF scene graph
struct Node 
{
	Node* parent;
	std::vector<Node*> children;
	glm::mat4 matrix;
	~Node() 
	{
		for (auto& child : children) 
		{
			delete child;
		}
	}
};

struct MaterialFactors
{
	unsigned int base_color;
	unsigned int diffuse;
};

struct TransformDataUbo
{
	glm::mat4 mvp;
	glm::mat4 model;
	glm::vec4 bbox_min_WS;
	glm::vec4 bbox_max_WS;
	glm::mat4 pad0;
	glm::vec4 pad1;
	glm::vec4 pad2;
};

struct TransformData
{
	glm::mat4 model = glm::identity<glm::mat4>();
};

enum class DrawFlag
{
	NONE = 1 << 0,
	VISIBLE = 1 << 1,
	CAST_SHADOW = 1 << 2,
};

inline DrawFlag operator|(DrawFlag a, DrawFlag b)
{
	return static_cast<DrawFlag>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&&(DrawFlag a, DrawFlag b)
{
	return (static_cast<int>(a) & static_cast<int>(b)) == static_cast<int>(b);
}





