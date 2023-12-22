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
	glm::vec4 position;
	glm::vec4 color;
	float radius;
	glm::vec3 padding;
};

struct point_lights
{
	std::vector<glm::vec4> position;
	std::vector<glm::vec4> color;
	std::vector<float> radius;
	std::vector<glm::vec3> padding;
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
	point_lights point_lights;

	size_t max_point_lights = 4096;
};

