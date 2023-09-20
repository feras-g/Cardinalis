#include "RenderObjectManager.h"
#include "VulkanRenderInterface.h"
#include "VulkanMesh.h"

void ObjectManager::add_mesh(const VulkanMesh& mesh, std::string_view mesh_name, const Transform& transform)
{
	m_mesh_id_from_name.insert({ mesh_name.data(), m_meshes.size()});
	m_meshes.push_back(mesh);

	InstanceData data;
	data.model = transform.compute_model_matrix();

	add_instance(mesh_name, data, "BaseInstance");
}

void ObjectManager::add_instance(std::string_view mesh_name, InstanceData data, std::string_view instance_name)
{
	if (m_mesh_id_from_name.contains(mesh_name.data()))
	{
		size_t mesh_idx = m_mesh_id_from_name.at(mesh_name.data());
		m_mesh_instance_data[mesh_idx].instances.push_back(data);
	}
}

void ObjectManager::create_mesh_ssbos()
{
	size_t num_meshes = m_mesh_instance_data.size();
	m_mesh_ssbo.resize(num_meshes);
	for (int mesh_idx = 0; mesh_idx < num_meshes; mesh_idx++)
	{
		std::string buf_name = "ShaderInstanceData for Mesh #" + std::to_string(mesh_idx);
		size_t buf_size_bytes = m_mesh_instance_data[mesh_idx].instances.size() * sizeof(InstanceData);

		m_mesh_ssbo[mesh_idx].init(Buffer::Type::STORAGE, buf_size_bytes, buf_name.c_str());
		m_mesh_ssbo[mesh_idx].upload(context.device, m_mesh_instance_data[mesh_idx].instances.data(), 0, buf_size_bytes);
	}
}
