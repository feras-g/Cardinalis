#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"

void LightManager::init()
{
	/* Create and fill UBO data */
	create_uniform_buffer(m_uniform_buffer, sizeof(m_light_data));
	update_ubo(nullptr);
}

void LightManager::update_ubo(LightData* data)
{
	if (data == nullptr)
	{
		data = &m_light_data;
	}

	void* pMappedData = nullptr;
	VK_CHECK(vkMapMemory(context.device, m_uniform_buffer.memory, 0, sizeof(LightData), 0, &pMappedData));
	memcpy(pMappedData, data, sizeof(LightData));
	vkUnmapMemory(context.device, m_uniform_buffer.memory);
}

void LightManager::destroy()
{
	destroy_buffer(m_uniform_buffer);
}