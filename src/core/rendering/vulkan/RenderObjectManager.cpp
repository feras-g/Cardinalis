#include "RenderObjectManager.h"
#include "VulkanRenderInterface.h"
#include "VulkanMesh.h"
#include "VulkanRendererBase.h"
#include "core/engine/Image.h"
#include "core/rendering/vulkan/Renderers/IRenderer.h"
#include "core/engine/vulkan/objects/vk_descriptor_set.hpp"

using namespace vk;

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

	m_materials_ssbo.upload(ctx.device, &m_materials.back(), material_idx * sizeof(Material), sizeof(Material));

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

size_t ObjectManager::add_mesh(const VulkanMesh& mesh, std::string_view mesh_name, const Transform& transform, bool add_base_instance)
{
	size_t mesh_idx = m_meshes.size();
	m_mesh_names.push_back(mesh_name.data());
	m_mesh_id_from_name.insert({ mesh_name.data(), mesh_idx });
	m_meshes.push_back(mesh);

	GPUInstanceData data
	{
		.model = mesh.model * glm::mat4(transform)
	};
	
	create_instance_buffer();
	
	if (add_base_instance)
	{
		add_mesh_instance(mesh_name, data, "BaseInstance");
	}

	vk::descriptor_set descriptor_set;
	descriptor_set.assign_layout(mesh_descriptor_set_layout);
	descriptor_set.create("Mesh Descriptor Set");
	descriptor_set.write_descriptor_storage_buffer(0, mesh.m_vertex_index_buffer, 0, mesh.m_vertex_buf_size_bytes);
	descriptor_set.write_descriptor_storage_buffer(1, mesh.m_vertex_index_buffer, mesh.m_vertex_buf_size_bytes, mesh.m_index_buf_size_bytes);
	descriptor_set.write_descriptor_storage_buffer(2, m_mesh_instance_data_ssbo[mesh_idx], 0, max_instance_count * sizeof(GPUInstanceData));
	descriptor_set.write_descriptor_storage_buffer(3, m_materials_ssbo, mesh.geometry_data.primitives[0].material_id * sizeof(Material), sizeof(Material));

	m_descriptor_sets.push_back(descriptor_set);


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
			m_mesh_instance_data_ssbo[mesh_idx].upload(ctx.device, &m_mesh_instance_data[mesh_idx].back(), offset, sizeof(data));
		}
	}
}

void ObjectManager::update_instances_ssbo(std::string_view mesh_name)
{
	if (m_mesh_id_from_name.contains(mesh_name.data()))
	{
		size_t mesh_idx = m_mesh_id_from_name.at(mesh_name.data());
		m_mesh_instance_data_ssbo[mesh_idx].upload(ctx.device, m_mesh_instance_data[mesh_idx].data(), 0, m_mesh_instance_data[mesh_idx].size() * sizeof(GPUInstanceData));
	}
}

void ObjectManager::init()
{
	create_materials_ssbo();

	/*
		Mesh descriptor set layout
	*/


	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_instance_count},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_instance_count},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_instance_count},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, max_instance_count},

	};

	mesh_descriptor_set_layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT, "Vertex Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(1, VK_SHADER_STAGE_VERTEX_BIT, "Index Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(2, VK_SHADER_STAGE_VERTEX_BIT, "Instance Buffer");
	mesh_descriptor_set_layout.add_storage_buffer_binding(3, VK_SHADER_STAGE_FRAGMENT_BIT, "Material Buffer");
	mesh_descriptor_set_layout.create("Instanced Mesh Descriptor Layout");

	create_textures_descriptor_set();
}


void ObjectManager::create_instance_buffer()
{
	size_t num_meshes = m_mesh_instance_data.size();
	
	std::string buf_name = "InstanceData for Mesh #" + std::to_string(std::max(0ull, m_meshes.size() - 1));

	m_mesh_instance_data.resize(max_instance_count);

	size_t buf_size_bytes = max_instance_count * sizeof(GPUInstanceData);

	vk::buffer instance_ssbo;
	instance_ssbo.init(vk::buffer::type::STORAGE, buf_size_bytes, buf_name.c_str());
	instance_ssbo.create();
	m_mesh_instance_data_ssbo.push_back(instance_ssbo);
}

void ObjectManager::create_materials_ssbo()
{
	size_t buf_size_bytes = max_material_count * sizeof(Material);
	std::string buf_name = "Materials";

	m_materials_ssbo.init(vk::buffer::type::STORAGE, buf_size_bytes, buf_name.c_str());
	m_materials_ssbo.create();

	// Add default material
	add_material(s_default_material);
}

void ObjectManager::create_textures_descriptor_set()
{
	texture_descriptor_array_binding = 0;
	m_bindless_layout.add_combined_image_sampler_binding(texture_descriptor_array_binding, VK_SHADER_STAGE_FRAGMENT_BIT, max_bindless_textures, "Bindless Textures");

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_flags_info = {};
	m_bindless_layout.binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);

	m_bindless_layout.create("Bindless Textures Descriptor Layout");

	m_descriptor_set_bindless_textures.assign_layout(m_bindless_layout);
	m_descriptor_set_bindless_textures.create("Bindless Textures Descriptor Set");

}

void ObjectManager::draw_mesh_list(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list, VkPipelineLayout pipeline_layout, std::span<VkDescriptorSet> additional_bound_descriptor_sets, DrawMetricsEntry& renderer_draw_metrics)
{
	for (size_t mesh_idx : mesh_list)
	{
		/* Mesh descriptor set must always be the first */
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, m_descriptor_sets[mesh_idx], 0, nullptr);

		const VulkanMesh& mesh = m_meshes[mesh_idx];

		uint32_t instance_count = (uint32_t)m_mesh_instance_data[mesh_idx].size();
		renderer_draw_metrics.increment_instance_count(instance_count);

		for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
		{
			const Primitive& p = mesh.geometry_data.primitives[prim_idx];
			vkCmdPushConstants(cmd_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &p.model);
			vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
			renderer_draw_metrics.increment_drawcall_count(1);
			renderer_draw_metrics.increment_vertex_count(p.vertex_count * instance_count);
		}
	}
}