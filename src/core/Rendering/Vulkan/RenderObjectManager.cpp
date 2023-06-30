#include "RenderObjectManager.h"

#include "VulkanMesh.h"

void ObjectManager::add_mesh(const VulkanMesh& mesh, const std::string& name)
{
	m_mesh_id_from_name.insert({ name, m_meshes.size() });
	m_meshes.push_back(mesh);

//	InstanceData instance_data;
//	instance_data.position = mesh.model;
//	m_instances.push_back({}};
}

void ObjectManager::add_instance(const std::string& mesh_name, const std::string& name)
{

}

