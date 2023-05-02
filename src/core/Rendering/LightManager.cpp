#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"

void LightManager::init()
{
	/* Create and fill UBO data */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		create_uniform_buffer(m_ubo[frame_idx], sizeof(m_light_data));
	}
	update(nullptr);
}

void LightManager::update(LightData* data)
{
	update_ubo(data);
	update_view_proj();
}

void LightManager::update_ubo(LightData* data)
{
	if (data == nullptr)
	{
		data = &m_light_data;
	}

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		void* pMappedData = nullptr;
		VK_CHECK(vkMapMemory(context.device, m_ubo[frame_idx].memory, 0, sizeof(LightData), 0, &pMappedData));
		memcpy(pMappedData, data, sizeof(LightData));
		vkUnmapMemory(context.device, m_ubo[frame_idx].memory);
	}

}

void LightManager::update_view_proj()
{
	m_light_data.directional_light.view_proj = compute_view_proj(eye, m_light_data.directional_light.direction, up, view_volume_bbox_min, view_volume_bbox_max);
}
void LightManager::destroy()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		destroy_buffer(m_ubo[frame_idx]);
	}
}

glm::mat4 compute_view_proj(glm::vec3 eye, glm::vec3 forward, glm::vec3 up, glm::vec3 view_volume_bbox_min, glm::vec3 view_volume_bbox_max)
{
	/* View volume */
	/*const glm::vec3 view_volume_bbox_min = { -10, -10, 0.0f };
	const glm::vec3 view_volume_bbox_max = { 10,  10,  10.0f };*/

	/* Compute projection matrix */
	glm::mat4 proj = glm::ortho(
		view_volume_bbox_min.x, view_volume_bbox_max.x, 
		view_volume_bbox_min.y, view_volume_bbox_max.y, 
		view_volume_bbox_min.z, view_volume_bbox_max.z
	);

	/* Compute view matrix */
	glm::mat4 view = glm::lookAt(20.0f * eye, forward, { 0.0f, 1.0f, 0.0f });
	
	// Light rays are parallel, we need an orthographic projection
	return proj * view;
}