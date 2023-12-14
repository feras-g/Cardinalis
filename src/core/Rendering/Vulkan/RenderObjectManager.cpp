#include "RenderObjectManager.h"
#include "VulkanRenderInterface.h"
#include "VulkanMesh.h"
#include "VulkanRendererBase.h"

#include "core/engine/Image.h"

uint32_t ObjectManager::add_material(const Material& material)
{
	size_t hash = MaterialHash{}(material);

	auto ite = m_material_id_from_hash.find(hash);

	if (ite != m_material_id_from_hash.end())
	{
		return ite->second;
	}

	uint32_t material_idx = (uint32_t)m_materials.size();
	m_materials.push_back(material);

	m_material_id_from_hash.insert({ hash, material_idx });

	m_materials_ssbo.upload(context.device, &m_materials.back(), material_idx * sizeof(Material), sizeof(Material));

	return material_idx;
}

int ObjectManager::add_texture(const Texture2D& texture)
{
	auto ite = m_texture_id_from_name.find(texture.info.debugName);

	if (ite != m_texture_id_from_name.end())
	{
		return ite->second;
	}

	int texture_idx = (int)m_textures.size();
	m_textures.push_back(texture);
	m_texture_id_from_name.insert({ texture.info.debugName, texture_idx });

	m_descriptor_set_bindless_textures.write_descriptor_combined_image_sampler(texture_descriptor_array_binding, texture.view, texture.sampler, texture_idx, 1);

	return texture_idx;
}

int ObjectManager::get_texture_id(std::string_view name)
{
	auto ite = m_texture_id_from_name.find(name.data());
	if (ite != m_texture_id_from_name.end())
	{
		return ite->second;
	}
	return -1;
}

size_t ObjectManager::add_mesh(const VulkanMesh& mesh, std::string_view mesh_name, const Transform& transform)
{
	size_t mesh_idx = m_meshes.size();
	m_mesh_id_from_name.insert({ mesh_name.data(), mesh_idx });
	m_meshes.push_back(mesh);

	GPUInstanceData data
	{
		.model = mesh.model * glm::mat4(transform)
	};
	
	create_instance_buffer();
	
	add_mesh_instance(mesh_name, data, "BaseInstance");

	DescriptorSet descriptor_set;
	descriptor_set.assign_layout(mesh_descriptor_set_layout);
	descriptor_set.create(m_descriptor_pool, "");
	descriptor_set.write_descriptor_storage_buffer(0, mesh.m_vertex_index_buffer, 0, mesh.m_vertex_buf_size_bytes);
	descriptor_set.write_descriptor_storage_buffer(1, mesh.m_vertex_index_buffer, mesh.m_vertex_buf_size_bytes, mesh.m_index_buf_size_bytes);
	descriptor_set.write_descriptor_storage_buffer(2, m_mesh_instance_data_ssbo[mesh_idx], 0, max_instance_count * sizeof(GPUInstanceData));
	descriptor_set.write_descriptor_storage_buffer(3, m_materials_ssbo, mesh.geometry_data.primitives[0].material_id * sizeof(Material), sizeof(Material));

	m_descriptor_sets.push_back(descriptor_set);

	if (m_mesh_id_from_name.contains(mesh_name.data()))
	{
		size_t mesh_idx = m_mesh_id_from_name.at(mesh_name.data());
		size_t offset = std::max(0ull, (m_mesh_instance_data[mesh_idx].size() - 1) * sizeof(GPUInstanceData));

		if ((offset + sizeof(GPUInstanceData)) < (max_instance_count * sizeof(GPUInstanceData)))
		{
			m_mesh_instance_data_ssbo[mesh_idx].upload(context.device, &m_mesh_instance_data[mesh_idx].back(), offset, sizeof(GPUInstanceData));
		}
	}

	return mesh_idx;
}

void ObjectManager::add_mesh_instance(std::string_view mesh_name, GPUInstanceData data, std::string_view instance_name)
{
	if (m_mesh_id_from_name.contains(mesh_name.data()))
	{
		size_t mesh_idx = m_mesh_id_from_name.at(mesh_name.data());
		m_mesh_instance_data[mesh_idx].push_back(data);
		size_t offset = std::max(0ull, (m_mesh_instance_data[mesh_idx].size() - 1) * sizeof(data));

		if ((offset + sizeof(data)) < (max_instance_count * sizeof(data)))
		{
			m_mesh_instance_data_ssbo[mesh_idx].upload(context.device, &m_mesh_instance_data[mesh_idx].back(), offset, sizeof(data));
		}
	}
}

void ObjectManager::update_instances_ssbo(std::string_view mesh_name)
{
	if (m_mesh_id_from_name.contains(mesh_name.data()))
	{
		size_t mesh_idx = m_mesh_id_from_name.at(mesh_name.data());
		m_mesh_instance_data_ssbo[mesh_idx].upload(context.device, m_mesh_instance_data[mesh_idx].data(), 0, m_mesh_instance_data[mesh_idx].size() * sizeof(GPUInstanceData));
	}
}

void ObjectManager::init()
{
	create_materials_ssbo();
	
	/* Create a default material */
	add_material({-1, -1, -1, -1});

	/*
		Mesh descriptor set layout
	*/
	m_descriptor_pool = create_descriptor_pool(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT, max_instance_count, 0, 0, max_bindless_textures, max_mesh_count);

	mesh_descriptor_set_layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT, "Vertex Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(1, VK_SHADER_STAGE_VERTEX_BIT, "Index Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(2, VK_SHADER_STAGE_VERTEX_BIT, "Instance Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(3, VK_SHADER_STAGE_FRAGMENT_BIT, "Material Buffer");
	mesh_descriptor_set_layout.create("Instanced Mesh Descriptor Layout");

	create_textures_descriptor_set(m_descriptor_pool);
}


void ObjectManager::create_instance_buffer()
{
	size_t num_meshes = m_mesh_instance_data.size();
	
	std::string buf_name = "InstanceData for Mesh #" + std::to_string(std::max(0ull, m_meshes.size() - 1));

	m_mesh_instance_data.resize(max_instance_count);

	size_t buf_size_bytes = max_instance_count * sizeof(GPUInstanceData);

	Buffer instance_ssbo;
	instance_ssbo.init(Buffer::Type::STORAGE, buf_size_bytes, buf_name.c_str());
	instance_ssbo.create();
	m_mesh_instance_data_ssbo.push_back(instance_ssbo);
}

void ObjectManager::create_materials_ssbo()
{
	size_t buf_size_bytes = max_material_count * sizeof(Material);
	std::string buf_name = "Materials";
	m_materials.resize(max_material_count);

	// Default material
	m_materials[0] = { -1, -1, -1, -1, {1.0f, 0.0f, 1.0f, 1.0f}, { 0.0f, 1.0f } };

	m_materials_ssbo.init(Buffer::Type::STORAGE, buf_size_bytes, buf_name.c_str());
	m_materials_ssbo.create();
}

void ObjectManager::create_textures_descriptor_set(VkDescriptorPool pool)
{
	texture_descriptor_array_binding = 0;
	m_bindless_layout.add_combined_image_sampler_binding(texture_descriptor_array_binding, VK_SHADER_STAGE_FRAGMENT_BIT, max_bindless_textures, "Bindless Textures");

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_flags_info = {};
	m_bindless_layout.binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);

	m_bindless_layout.create("Bindless Textures Descriptor Layout");

	m_descriptor_set_bindless_textures.assign_layout(m_bindless_layout);
	m_descriptor_set_bindless_textures.create(m_descriptor_pool, "Bindless Textures Descriptor Set");

}