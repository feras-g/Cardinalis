#pragma once

#include "IRenderer.h"

struct DebugLineRenderer : public IRenderer
{
	void init() override
	{
		name = "Shadow Renderer";
		create_renderpass();
		create_pipeline();
	}

	void create_pipeline() override
	{

	}

	void create_renderpass() override
	{

	}

	void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
	{

	}

	void render(VkCommandBuffer cmd_buffer) override
	{
	}

	void on_window_resize()
	{
		create_renderpass();
	}

	virtual void show_ui()
	{

	}

	virtual bool reload_pipeline()
	{
		return false;
	}

	/* Shadow specific */
	/*
		z_near			Camera near clip.
		z_far			Camera far clip.
		fov				Camera field of view.
		aspect_ratio	Camera aspect ratio.
		lambda			Factor for interpolation between uniform and logarithmic cascade split distribution.
	
	*/
	void compute_cascade_splits(float z_near, float z_far, float fov, float aspect_ratio, float lambda)
	{
		// Use Practical Split Scheme method
		// https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf, page 6
		float z_range = z_far - z_near;
		float z_min = z_near;
		float z_max = z_near + z_range;
		float z_ratio = z_max / z_min;

		float prev_z_split = 0.0f;
		for (int i = 0; i < k_num_cascades; ++i)
		{
			float p = (i + 1) / (float)(k_num_cascades);
			float log = z_min * std::pow(z_ratio, p);
			float uniform = z_min + z_range * p;
			float z = lambda * (log - uniform) + uniform;
			z_split_cascade[i] = (z - z_near) / z_range;
			projection_cascade[i] = glm::perspective(glm::radians(fov), aspect_ratio, (z_near + prev_z_split * z_range), (z_near + z_split_cascade[i] * z_range));
			prev_z_split = z_split_cascade[i];
		}
	}


	static constexpr int k_num_cascades = 4;
	float z_split_cascade[k_num_cascades];			// Distance from near-plane for each cascade
	glm::mat4 projection_cascade[k_num_cascades];
	Texture2D shadow_cascades_depth[NUM_FRAMES];	// Each element is a Texture2DArray storing k_num_cascades cascades
	renderpass_dynamic renderpass[NUM_FRAMES];
};