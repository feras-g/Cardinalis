#pragma once

#include "IRenderer.h"

struct VolumetricLightRenderer : IRenderer
{
	virtual void init()
	{
		
	}

	virtual void create_pipeline()
	{

	}

	virtual void create_renderpass()
	{

	}

	virtual void render(VkCommandBuffer cmd_buffer)
	{

	}

	virtual void show_ui() {}
	virtual bool reload_pipeline() {}
};