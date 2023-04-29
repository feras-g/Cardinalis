#pragma once
#include "Vulkan/VulkanResources.h"
#include <glm/vec4.hpp>

struct PointLight
{
	glm::vec4 position{};
	glm::vec4 color{};
	glm::vec4 props{};
};

struct DirectionalLight
{
	glm::vec4 position{ 0, 1, 0, 1 };
	glm::vec4 color{};
};

struct LightData
{
	DirectionalLight directional_light;
};

struct LightManager
{
	void init();
	void update_ubo(LightData* data);
	void destroy();

	LightData m_light_data;
	Buffer m_uniform_buffer;
};