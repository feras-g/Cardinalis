#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"
#include <random>

void LightManager::init()
{
	int rows = 20;
	int cols = 20;
	light_data.num_point_lights = rows * cols;
	light_data.point_lights.resize(light_data.num_point_lights);

	for (int y = 0; y < rows; y++)
	for (int x = 0; x < cols; x++)
	{
		//int square_size = 20;
		//std::uniform_real_distribution<float> distribution(0.0f, static_cast<float>(square_size));
		//float x = distribution(gen) - square_size/2;
		//float z = distribution(gen) - square_size/2;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> color_dist(0.0f, static_cast<float>(1.0f));
		float r = color_dist(gen);
		float g = color_dist(gen);
		float b = color_dist(gen);

		float spacing = 1.5;

		float px = (- (cols / 2) + x) * spacing;
		float py = (- (rows / 2) + y) * spacing;


		float radius = 1.0f;

		light_data.point_lights[x + (y * rows) ] =
		{
			{ px, 1.0f, py, 1.0f }, /* Position*/
			{ r, g, b, 1.0f },	  /* Color */
			radius,				  /* Radius */
		};
	}

	light_data_size = sizeof(light_data.directional_light) + sizeof(unsigned) + 3 * sizeof(float) + light_data.point_lights.size() * sizeof(PointLight);

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
		offset += sizeof(light_data.num_point_lights) + sizeof(light_data.pad);
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

