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
	float radius;
	glm::vec3 color;
	float pad1;
};


struct light_manager
{
	void init();

	void create_ssbo();
	void write_ssbo();
	void create_descriptor_set();
	static void create_light_volumes();
	static void add_point_light(point_light p);
	static void set_directional_light(directional_light d);

	void update_dir_light(directional_light dir_light);
	void add_test_point_lights();
	void show_ui();

	VkDescriptorPool descriptor_pool;
	static inline std::array<vk::descriptor_set, NUM_FRAMES> descriptor_set;
	static inline std::array<vk::buffer, NUM_FRAMES> ssbo;
	static inline vk::descriptor_set_layout descriptor_set_layout;

	static inline directional_light dir_light;
	static inline std::vector<point_light> point_lights;

	size_t max_point_lights = 20000;

	/* Light volumes */
	static inline size_t point_light_volume_mesh_id;
	static inline size_t directional_light_volume_mesh_id;
};

