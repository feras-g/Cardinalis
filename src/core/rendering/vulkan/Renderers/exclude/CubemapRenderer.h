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
	void render_cubemap(VkCommandBuffer cmd_buffer, VkRect2D area);
	void render_skybox(size_t currentImageIdx, VkCommandBuffer cmd_buffer);
	void init_skybox_rendering();
	void init_irradiance_map_rendering();
	void render_irradiance_map(VkCommandBuffer cmd_buffer);

	static inline VkImageView tex_cubemap_view;
	static inline VkImageView tex_irradiance_map_view;

	/* Handle to equirectangular HDR environment texture */
	size_t equirect_map_id = 0;
};