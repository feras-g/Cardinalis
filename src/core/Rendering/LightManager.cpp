#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"

void LightManager::init()
{
	/* Create and fill UBO data */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_ubo[frame_idx].init(Buffer::Type::UNIFORM, sizeof(m_light_data));
	}
	update(nullptr, {-1,-1,-1}, { 1,1,1 });
}

void LightManager::update(LightData* data, glm::vec3 bbox_min, glm::vec3 bbox_max)
{
	update_ubo(data);
	update_view_proj(bbox_min, bbox_max);
}

void LightManager::update_ubo(LightData* data)
{
	if (data == nullptr)
	{
		data = &m_light_data;
	}

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_ubo[frame_idx].upload(context.device, data, 0, sizeof(LightData));
	}
}

void LightManager::update_view_proj(glm::vec3 bbox_min, glm::vec3 bbox_max)
{
	m_light_data.directional_light.view_proj = compute_view_proj(eye, m_light_data.directional_light.direction, up, bbox_min, bbox_max);
}
void LightManager::destroy()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_ubo[frame_idx].destroy();
	}
}

glm::mat4 compute_view_proj(glm::vec3 eye, glm::vec3 forward, glm::vec3 up, glm::vec3 view_volume_bbox_min, glm::vec3 view_volume_bbox_max)
{
	/* Temp */
	//eye = { 0.02f, 1.0f, 0.00001f };
	//forward = { 0, 1, 0 };
	//up = { 0.0f, 1.0f, 0.0f };

	/* View volume */
	/*const glm::vec3 view_volume_bbox_min = { -10, -10, 0.0f };
	const glm::vec3 view_volume_bbox_max = { 10,  10,  10.0f };*/

	/* Compute projection matrix */
	//LightManager::proj = glm::perspective(45.0f, 1.0f, view_volume_bbox_min.z, view_volume_bbox_max.z);
	//LightManager::proj = glm::ortho(
	//	view_volume_bbox_min.x, view_volume_bbox_max.x, 
	//	view_volume_bbox_min.y, view_volume_bbox_max.y, 
	//	view_volume_bbox_min.z, view_volume_bbox_max.z
	//);

	LightManager::direction = glm::normalize(forward);
	
	/* Compute view matrix */
	LightManager::view = glm::lookAt(eye, forward, up);
	
	// Light rays are parallel, we need an orthographic projection
	return LightManager::proj * LightManager::view;
}