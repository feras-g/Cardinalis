#pragma once

#include "lighting.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanUI.h"

void light_manager::init()
{
	dir_light.color = glm::vec4(0.8, 0.4, 0.2, 1.0);
	dir_light.dir	= glm::normalize(glm::vec4(0.5f, 0.5f, 0.5f, 0.0));

	size_t point_light_size = 3 * sizeof(glm::vec4);
	ssbo.init(vk::buffer::type::STORAGE, max_point_lights * point_light_size + sizeof(directional_light), "SSBO Lighting");
	ssbo.create();

	update_dir_light(dir_light);

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

void light_manager::update_dir_light(directional_light dir_light)
{
	this->dir_light = dir_light;
	ssbo.upload(ctx.device, &this->dir_light, 0, sizeof(directional_light));
}

void light_manager::update_point_lights()
{

}

void light_manager::show_ui()
{
	if (ImGui::Begin("Lighting"))
	{
		directional_light prev = dir_light;
		ImGui::SeparatorText("Directional Light");
		ImGui::InputFloat4("Direction", glm::value_ptr(dir_light.dir));
		ImGui::ColorPicker4("Color", glm::value_ptr(dir_light.color));

		if (dir_light.color != prev.color || dir_light.dir != prev.dir )
		{
			update_dir_light(dir_light);
		}
	}

	ImGui::End();
}
