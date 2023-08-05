#include "VulkanResources.h"

#include "glm/vec3.hpp"
#include <unordered_map>

struct VulkanMesh;

class ObjectManager
{
public:
	struct InstanceData
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	void add_mesh(const VulkanMesh& mesh, const std::string& name);
	void add_instance(const std::string& mesh_name, const std::string& instance_name = "");

	std::unordered_map < std::string, size_t > m_mesh_id_from_name;
	std::vector<VulkanMesh>   m_meshes;
	std::vector<std::vector<InstanceData>> m_instances;
	std::vector<Buffer> 	  m_instance_buffers;
};