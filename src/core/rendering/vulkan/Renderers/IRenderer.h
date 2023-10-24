#pragma once

#include <vulkan/vulkan_core.h>
#include "core/rendering/vulkan/DescriptorSet.h"
#include "core/rendering/vulkan/VulkanShader.h"

struct IRenderer
{
	/* Call all create* functions */
	virtual void init() = 0;

	virtual void create_pipeline() = 0;

	virtual void create_renderpass() = 0;

	virtual void render(VkCommandBuffer cmd_buffer) = 0;

	void on_window_resize()
	{
		create_renderpass();
	}
};
