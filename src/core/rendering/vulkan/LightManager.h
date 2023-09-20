#pragma once
#include "core/rendering/vulkan/VulkanResources.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct PointLight
{
	glm::vec4 position;
	glm::vec4 color;
	float radius;
	float pad[3];
};

struct DirectionalLight
{
	glm::vec4 direction{ -0.58f, -0.58f, 0.58f, 0.0f };
	glm::vec4 color{ 0.8f, 0.4f, 0.2f, 1.0 };
};

struct LightData
{
	uint32_t num_point_lights;
	uint32_t pad[3];
	DirectionalLight directional_light;
	std::vector<PointLight> point_lights;
};

struct LightManager
{
	void init();
	void update(float time, float delta_time);
	void update_ssbo();
	void destroy();

	/* Cycles directional light direction with time. */
	bool b_cycle_directional_light = false;
	float cycle_speed = 1.0f;
	void cycle_directional_light(float delta_time);

	/* Animate point lights */
	bool b_animate_point_lights = false;
	void animate_point_lights(float time);
	float anim_freq = 0.01f;

	LightData light_data;
	size_t light_data_size;

	Buffer ssbo_light_data[NUM_FRAMES];
};