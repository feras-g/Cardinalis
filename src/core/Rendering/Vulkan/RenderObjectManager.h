#pragma once 

#include "VulkanResources.h"
#include "DescriptorSet.h"
#include "../Material.hpp"

#include <unordered_map>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/ext/matrix_transform.hpp"

struct VulkanMesh;

struct Transform
{
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 scale    = { 1.0f, 1.0f, 1.0f };

	operator glm::mat4() const
	{
		glm::mat4 T = glm::translate(glm::identity<glm::mat4>(), position);
		T *= glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z));
		T = glm::scale(T, scale);
		return T;
	}
};

class ObjectManager
{
public:
	/* Add material to array and return its index */
	uint32_t add_material(const Material& material);
	std::vector<Material> m_materials;
	std::unordered_map<size_t, int> m_material_id_from_hash;

	Buffer m_materials_ssbo;

	int add_texture(const Texture2D& texture);

	/* Returns -1 if texture is not found */
	int get_texture_id(std::string_view name);
	std::vector<Texture2D> m_textures;
	std::vector<VkDescriptorImageInfo> m_texture_descriptors;
	std::unordered_map<std::string, int> m_texture_id_from_name;

	/* Shader side instance data */
	struct GPUInstanceData
	{
		glm::mat4 model;
		glm::vec4 color;
	};

	size_t add_mesh(const VulkanMesh& mesh, std::string_view mesh_name, const Transform& transform);
	void add_mesh_instance(std::string_view mesh_name,GPUInstanceData data, std::string_view instance_name = "");


	/* Updates the SSBO instance data for mesh correponding to mesh_name */
	void update_instances_ssbo(std::string_view mesh_name);

	std::unordered_map < std::string, size_t > m_mesh_id_from_name;
	std::vector<VulkanMesh>   m_meshes;

	VkDescriptorPool m_descriptor_pool;

	/*
		Each element is the descriptor set of a mesh
		Descriptor set layout is as follows :
			0: SSBO for Mesh Vertex Data
			1: SSBO for Mesh Index Data
			2: SSBO for Mesh Instance Data
			3: UBO for Mesh Material Data
	*/
	std::vector<DescriptorSet> m_descriptor_sets;

	/* Descriptor set holding array of texture descriptors */
	uint32_t texture_descriptor_array_binding = 0;
	DescriptorSet m_descriptor_set_bindless_textures;
	DescriptorSetLayout m_bindless_layout; 
	
	/* Each element corresponds to the instance data for a mesh */
	std::vector<std::vector<GPUInstanceData>> m_mesh_instance_data;


	/* Total pre-allocated number of resource */
	uint32_t max_instance_count = 32768;
	uint32_t max_mesh_count = 4096;
	uint32_t max_material_count  = 4096;
	uint32_t max_bindless_textures  = 4096;
	uint32_t default_material_id	 = 0;

	/* Store for mesh at index i an SSBO containing the shader data for all instances of the mesh */
	std::vector<Buffer> 	  m_mesh_instance_data_ssbo;

	static inline DescriptorSetLayout mesh_descriptor_set_layout;

public:
	void init();

	static ObjectManager& get_instance()
	{
		if (nullptr == s_instance)
		{
			s_instance = new ObjectManager();
		}

		return *s_instance;
	}

protected:
	static inline ObjectManager* s_instance;

	/* Create an SSBO to store the instance for a mesh */
	void create_instance_buffer();

	/* Creates the SSBO storing all materials */
	void create_materials_ssbo();

	/* Creates the SSBO storing all texture descriptors */
	void create_textures_descriptor_set(VkDescriptorPool pool);
private:
	ObjectManager() = default;

};

