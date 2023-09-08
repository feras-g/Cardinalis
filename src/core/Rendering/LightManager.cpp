#include "LightManager.h"
#include "Vulkan/VulkanRenderInterface.h"
#include <random>

void LightManager::init()
{
	int rows  = 20;
	int cols  = 20;
	int depth = 1;


	for (int z = 0; z < depth;z++)
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

		float px = (- (cols / 2.0f) + x) * spacing;
		float py = (- (rows / 2.0f) + y) * spacing;
		float pz = (- (depth / 2.0f) + z) * spacing;

		float radius = 1.0f;

		light_data.point_lights.push_back(
			{
				{ px, pz, py, 1.0f }, /* Position*/
				{ r, g, b, 1.0f },	  /* Color */
				radius,				  /* Radius */
			}
		);
	}

	light_data.num_point_lights = (uint32_t)light_data.point_lights.size();
	light_data_size = sizeof(light_data.directional_light) + sizeof(unsigned) + 3 * sizeof(float) + light_data.point_lights.size() * sizeof(PointLight);

	/* Create and fill UBO data */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		ssbo_light_data[frame_idx].init(Buffer::Type::STORAGE, light_data_size, "Light Data Storage Buffer");
	}
	update_ssbo();
}

void LightManager::update(float time, float delta_time)
{
	if (b_cycle_directional_light)
	{
		cycle_directional_light(delta_time);
	}

	if (b_animate_point_lights)
	{
		animate_point_lights(time);
	}
}

void LightManager::update_ssbo()
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

void LightManager::cycle_directional_light(float delta_time)
{
	float rot_angle_per_frame = cycle_speed * glm::radians(1.0f) * delta_time;
	glm::mat4 rot_mat = glm::rotate(glm::mat4(1.0f), rot_angle_per_frame, glm::vec3(0.0f, 1.0f, 0.0f));
	light_data.directional_light.direction = rot_mat * light_data.directional_light.direction;
	update_ssbo();
}

void LightManager::animate_point_lights(float time)
{
	for (unsigned i = 0; i < light_data.num_point_lights; i++)
	{
		float id = i / (float)light_data.num_point_lights;
		light_data.point_lights[i].position.x += sin(time * anim_freq * id) * 0.01f;
		light_data.point_lights[i].position.z += cos(time * anim_freq * id) * 0.01f;
	}
	update_ssbo();
}

