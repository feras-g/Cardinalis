#pragma once
#include "Vulkan/VulkanResources.h"
#include "Vulkan/VulkanRenderInterface.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct PointLight
{
	//unsigned int num_lights;
	//float pad[3];
	glm::vec4 position;
	glm::vec4 color;
};

struct DirectionalLight
{
	glm::vec4 direction{ -0.58f, -0.58f, 0.58f, 0.0f };
	glm::vec4 color{ 80.f, 40.f, 20.f, 1.0 };
};

struct LightData
{
	unsigned num_point_lights;
	DirectionalLight directional_light;
	std::vector<PointLight> point_lights;
};

struct LightManager
{
	void init();
	void update(LightData* data);
	void destroy();

	LightData light_data;
	size_t light_data_size;

	Buffer ssbo_light_data[NUM_FRAMES];
};