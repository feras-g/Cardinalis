#include "VulkanResources.h"

#include <unordered_map>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/ext/matrix_transform.hpp"

struct VulkanMesh;

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	inline glm::mat4 compute_model_matrix() const
	{
		glm::mat4 T = glm::identity<glm::mat4>();
		T = glm::translate(T, position)
			* glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z))
			* glm::scale(T, scale);
		return T;
	}
};

class ObjectManager
{
public:
	/* Data for a single instance */
	struct InstanceData
	{
		glm::mat4 model;
	};

	/* Data for a mesh and its instances */
	struct MeshData
	{
		std::vector<InstanceData> instances;
	};

	void add_mesh(const VulkanMesh& mesh, std::string_view mesh_name, const Transform& transform);
	void add_instance(std::string_view mesh_name, InstanceData data, std::string_view instance_name = "");

	/* Create for each model an SSBO containing the model data */
	void create_mesh_ssbos();

	std::unordered_map < std::string, size_t > m_mesh_id_from_name;
	std::vector<VulkanMesh>   m_meshes;

	/* Store for mesh at index i an array containing the data for all instances of the mesh */
	std::vector<MeshData> m_mesh_instance_data;

	/* Store for mesh at index i an SSBO containing the shader data for all instances of the mesh */
	std::vector<Buffer> 	  m_mesh_ssbo;
};

