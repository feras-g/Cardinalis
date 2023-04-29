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
	size_t material_id;
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
	void create_from_file(const std::string& filename);
	void create_from_file_gltf(const std::string& filename);
	void create_from_data(std::span<SimpleVertexData> vertices, std::span<unsigned int> indices);
	size_t m_vertex_buf_size_bytes;
	size_t m_index_buf_size_bytes;
	size_t m_num_vertices;
	size_t m_num_indices;

	/* Non-interleaved vertex and index data. Used for vertex pulling. */
	Buffer m_vertex_index_buffer;
	VkDescriptorSet descriptor_set;

	GeometryData geometry_data{};
};


constexpr VkFormat tex_base_color_format         = VK_FORMAT_R8G8B8A8_SRGB;
constexpr VkFormat tex_metallic_roughness_format = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat tex_normal_format             = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat tex_emissive_format           = VK_FORMAT_R8G8B8A8_SRGB;

struct MaterialFactors
{
	unsigned int base_color;
	unsigned int diffuse;
};

struct Material
{
	unsigned int tex_base_color_id;
	unsigned int tex_metallic_roughness_id;
	unsigned int tex_normal_id;
	unsigned int tex_emissive_id;

	glm::vec4 base_color_factor;
	float metallic_factor;
	float roughness_factor;
};

template<>
struct std::hash<Material> 
{
	std::size_t operator()(const Material& s, std::size_t seed = 0) const
	{
		std::hash<unsigned int> hash0;
		std::hash<float> hash1;
		std::hash<glm::vec4> hash2;

		seed ^= hash0(s.tex_base_color_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash0(s.tex_metallic_roughness_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash0(s.tex_normal_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash0(s.tex_emissive_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash1(s.metallic_factor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash1(s.roughness_factor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash2(s.base_color_factor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

		return seed;
	}
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
	bool has_primitives = false;
	VulkanMesh* mesh_handle;
	Drawable(VulkanMesh* mesh, bool b_has_primitives = false);
	void draw(VkCommandBuffer cmd_buffer) const;
	void draw_primitives(VkCommandBuffer cmd_buffer) const;
};

struct Materials
{
	std::vector<std::string> names;
};




