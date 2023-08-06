#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"

void LightManager::init()
{
	light_data.num_point_lights = 10;
	for (unsigned i = 0; i < light_data.num_point_lights; i++)
	{
		light_data.point_lights.push_back
		(
			{
				{ i, 1, 0, 1 }, /* Position*/
				{ 1.0f, 0.0f, 0.0f, 1.0f } /* Color */
			}
		);
	}

	light_data_size = sizeof(light_data.directional_light) + sizeof(unsigned) + light_data.point_lights.size() * sizeof(PointLight);

	/* Create and fill UBO data */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		ssbo_light_data[frame_idx].init(Buffer::Type::STORAGE, light_data_size, "Light Data Storage Buffer");
	}
	update(nullptr);
}

void LightManager::update(LightData* data)
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		size_t directional_light_size = sizeof(DirectionalLight);

		VkDeviceSize offset = 0;
		ssbo_light_data[frame_idx].upload(context.device, &light_data.directional_light, offset, sizeof(DirectionalLight));
		offset += sizeof(DirectionalLight);
		ssbo_light_data[frame_idx].upload(context.device, &light_data.num_point_lights, offset, sizeof(unsigned));
		offset += sizeof(unsigned);
		ssbo_light_data[frame_idx].upload(context.device, light_data.point_lights.data(), offset, light_data.point_lights.size() * sizeof(PointLight));
	}
}

void LightManager::destroy()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		ssbo_light_data[frame_idx].destroy();
	}
}

