#pragma once
#include "Vulkan/VulkanResources.h"
#include "Vulkan/VulkanRenderInterface.h"
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
	void update(LightData* data);
	void destroy();

	LightData light_data;
	size_t light_data_size;

	Buffer ssbo_light_data[NUM_FRAMES];
};