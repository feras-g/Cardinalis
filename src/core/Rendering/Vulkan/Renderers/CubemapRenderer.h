#pragma once

#include <vulkan/vulkan.hpp>

struct CubemapRenderer
{
	void init();
	void render();
	void draw_scene();
	void load_resources(const char* filename);
	void render(VkCommandBuffer cmd_buffer, VkRect2D area);

	/* Handle to equirectangular HDR environment texture */
	size_t equirect_map_id = 0;
};