#pragma once

#include <vulkan/vulkan.hpp>

struct CubemapRenderer
{
	void init();
	void render();
	void draw_scene();
	void init_resources(const char* filename);
	void init_descriptors();
	void init_gfx_pipeline();
	void render(VkCommandBuffer cmd_buffer, VkRect2D area);
	void render_cubemap(VkRect2D area);

	/* Handle to equirectangular HDR environment texture */
	size_t equirect_map_id = 0;
};