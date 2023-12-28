#pragma once

#include "lighting.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanUI.h"
#include "core/rendering/vulkan/RenderObjectManager.h"
#include "core/rendering/vulkan/VulkanMesh.h"

static inline VulkanMesh point_light_volume;
static inline VulkanMesh directional_light_volume;

void light_manager::init()
{
	create_ssbo();
	create_descriptor_set();
	create_light_volumes();

	directional_light default_dir_light;
	default_dir_light.color = glm::vec4(0.8, 0.4, 0.2, 1.0);
	default_dir_light.dir = glm::normalize(glm::vec4(0.25f, -1.0f, 0.5f, 0.0));
	set_directional_light(default_dir_light);
	//add_test_point_lights();
}

void light_manager::create_descriptor_set()
{
	std::array<VkDescriptorPoolSize, 1> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
	};

	descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

	descriptor_set.layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, "DirectLightingDataBlock");
	descriptor_set.layout.create("Light Manager Layout");
	descriptor_set.create(descriptor_pool, "Light Manager");
	descriptor_set.write_descriptor_storage_buffer(0, ssbo, 0, VK_WHOLE_SIZE);
}

void light_manager::create_ssbo()
{
	ssbo.init(vk::buffer::type::STORAGE, max_point_lights * sizeof(point_light) + sizeof(directional_light), "SSBO Lighting");
	ssbo.create();
}
void light_manager::write_ssbo()
{
	ssbo.upload(ctx.device, &this->dir_light, 0, sizeof(directional_light));
	ssbo.upload(ctx.device, point_lights.data(), sizeof(directional_light), point_lights.size() * sizeof(point_light));
}

void light_manager::create_light_volumes()
{
	/* Point light */
	ObjectManager& object_manager = ObjectManager::get_instance();
	point_light_volume.create_from_file("basic/unit_sphere_ico.glb");
	point_light_volume_mesh_id = object_manager.add_mesh(point_light_volume, "Point Light Volume", {}, false);

	/* Directional Light */
	std::array<VertexData, 3> fullscreen_quad_vertices
	{
		VertexData{  { -1, -1, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },	// Lower-left
		VertexData{  { 3, -1, 0 }, { 0, 0, 0 }, { 1, 1, 0 } },	// Upper-Right
		VertexData{  { -1, 3 , 0 }, { 0, 0, 0 }, { 0, 1, 0 } },	// Upper-left
	};
	std::array<unsigned int, 3> fullscreen_quad_indices { /* CCW */ 0, 1, 2, };
	directional_light_volume.create_from_data(fullscreen_quad_vertices, fullscreen_quad_indices);
	directional_light_volume.model = glm::identity<glm::mat4>();
	directional_light_volume.geometry_data.primitives.push_back({0, 3, glm::identity<glm::mat4>(), 0});
	directional_light_volume_mesh_id = object_manager.add_mesh(directional_light_volume, "Directional Light Volume", {}, true);
}

void light_manager::update_dir_light(directional_light dir_light)
{
	this->dir_light = dir_light;
	ssbo.upload(ctx.device, &this->dir_light, 0, sizeof(directional_light));
}

void light_manager::add_point_light(point_light p)
{
	point_lights.push_back(p);

	/* Light volume */
	Transform t;
	t.position = p.position;
	t.scale = glm::vec3(p.radius);

	ObjectManager::GPUInstanceData instance_data;
	instance_data.model = glm::mat4(t);

	ObjectManager& object_manager = ObjectManager::get_instance();
	object_manager.add_mesh_instance("Point Light Volume", instance_data);
}

void light_manager::set_directional_light(directional_light d)
{
	dir_light.color = d.color; 
	dir_light.dir = d.dir; 
}

void light_manager::add_test_point_lights()
{
	const int x = 32;
	const int y = 32;

	for (int i = -x/2; i < x/2; i++)
	{
		for (int j = -y/2; j < y/2; j++)
		{
			point_light p;
			p.color = glm::linearRand(glm::vec3(0.1f), glm::vec3(1.0f));
			p.radius = 1.0f;
			p.position = glm::vec3(i * 2, 0.0, j);

			add_point_light(p);
		}
	}

	ssbo.upload(ctx.device, point_lights.data(), sizeof(directional_light), point_lights.size() * sizeof(point_light));
}

void light_manager::show_ui()
{
	if (ImGui::Begin("Lighting"))
	{
		directional_light prev = dir_light;
		ImGui::SeparatorText("Directional Light");
		ImGui::DragFloat4("Direction", glm::value_ptr(dir_light.dir), 0.005f, -1000, 1000);
		ImGui::ColorPicker4("Color", glm::value_ptr(dir_light.color));

		if (dir_light.color != prev.color || dir_light.dir != prev.dir )
		{
			update_dir_light(dir_light);
		}

		for (int i = 0; i < point_lights.size(); i++)
		{
			std::string name = "Position #" + std::to_string(i);
			ImGui::DragFloat3(name.c_str(), glm::value_ptr(point_lights[i].position), 0.005f, -1000, 1000);
			ImGui::DragFloat3("Color", glm::value_ptr(point_lights[i].color));
		}
	}

	ImGui::End();
}
