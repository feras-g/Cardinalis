#pragma once

#include "core/engine/common.h"
#include "core/engine/vulkan/objects/vk_buffer.h"
#include "core/engine/vulkan/objects/vk_descriptor_set.hpp"

struct directional_light
{
	glm::vec4 dir;
	glm::vec4 color;
};

struct point_light
{
	glm::vec3 position;
	float pad;
	glm::vec3 color;
	float radius;
};


struct light_manager
{
	void init();
	void update_dir_light(directional_light dir_light);
	void update_point_lights();
	void show_ui();

	VkDescriptorPool descriptor_pool;
	static inline vk::descriptor_set descriptor_set;
	vk::buffer ssbo;

	static inline directional_light dir_light;
	std::vector<point_light> point_lights;

	size_t max_point_lights = 8096;

	/* Light volumes */
	static inline size_t point_light_volume_mesh_id;
	static inline size_t directional_light_volume_mesh_id;
};

