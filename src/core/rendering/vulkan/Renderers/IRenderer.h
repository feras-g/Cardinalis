#pragma once

#include <string>

#include <vulkan/vulkan_core.h>
#include "core/engine/vulkan/objects/vk_descriptor_set.hpp"
#include "core/rendering/vulkan/VulkanShader.h"
#include "core/rendering/draw_metrics.h"
#include "core/engine/vulkan/objects/vk_debug_marker.hpp"
#include "core/rendering/vulkan/VulkanTexture.h"

#include "core/engine/vulkan/objects/vk_renderpass.h"

struct IRenderer
{
	/* Call all create* functions */
	virtual void init() = 0;
	virtual void create_pipeline() = 0;
	virtual void create_renderpass() = 0;
	virtual void render(VkCommandBuffer cmd_buffer) = 0;
	virtual void show_ui() = 0;
	virtual bool reload_pipeline() = 0;

	virtual void on_window_resize()
	{
		create_renderpass();
	}

	static inline VkPolygonMode global_polygon_mode = VK_POLYGON_MODE_FILL;
	Pipeline pipeline;
	VertexFragmentShader shader;
	VkFormat color_format;
	VkFormat depth_format;
	vk::renderpass_dynamic renderpass[NUM_FRAMES];
	std::string name;
};
