#pragma once
#include "Vulkan/VulkanResources.h"
#include "Vulkan/VulkanRenderInterface.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/* Compute view proj matrix for directional light */
static glm::mat4 compute_view_proj(glm::vec3 eye, glm::vec3 direction, glm::vec3 up, glm::vec3 view_volume_bbox_min, glm::vec3 view_volume_bbox_max);

struct PointLight
{
	glm::vec4 position{};
	glm::vec4 color{};
	glm::vec4 props{};
};

struct DirectionalLight
{
	glm::vec4 direction{ 0, 1, 0, 0 };
	glm::vec4 color{ 1.0 };
	glm::mat4 view_proj{};
};

struct LightData
{
	DirectionalLight directional_light;
};

struct LightManager
{
	void init();
	void update(LightData* data, glm::vec3 bbox_min, glm::vec3 bbox_max);
	void update_ubo(LightData* data);
	void destroy();
	void update_view_proj(glm::vec3 bbox_min, glm::vec3 bbox_max);

	LightData m_light_data;
	Buffer m_ubo[NUM_FRAMES];

	/* */
	glm::vec3 eye = { 0,0,0 };
	glm::vec3 up  = { 0,1,0 };
	glm::vec3 view_volume_bbox_min = { -20.0f, -20.0f, -20.0f };
	glm::vec3 view_volume_bbox_max = { 20.0f,  20.0f,   20.0f  };
	static inline glm::mat4 view;
	static inline glm::mat4 proj;
};