#pragma once

#include "IRenderer.h"

struct TwoPassGaussianBlur
{
	static constexpr int k_thread_group_size = 256;
	static constexpr int k_max_num_weights = 11;
	static constexpr const char* k_ps_range_name = "Blur Params";

	void init()
	{
		shader.create("two_pass_gaussian_blur_comp.comp.spv");

		descriptor_set.layout.add_storage_image_binding(0, "Source Read-Only Image");
		descriptor_set.layout.add_storage_image_binding(1, "TwoPassGaussianBlur Result Image");

		pipeline.layout.add_push_constant_range(k_ps_range_name, { .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .offset = 0, .size = sizeof(blur_params) });

		pipeline.create_compute(shader);
	}

	void set_source(const Texture2D& source)
	{
		// TODO: create storage image view
		// TODO: create blur destination image

		blur_params =
		{
			.inv_image_size = 1.0f / std::max(source.info.width, source.info.height),//TODO
			.weights = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162},
			.radius = 4,
		};
	}

	void execute(VkCommandBuffer cmd_buffer)
	{
		const int num_workgroups = std::max(source.info.width, source.info.height) / k_thread_group_size;
		// TODO: bind descriptor set

		pipeline.cmd_push_constants(cmd_buffer, k_ps_range_name, &blur_params);
		pipeline.bind(cmd_buffer);
		vkCmdDispatch(cmd_buffer, num_workgroups, num_workgroups, 1);
	}

	struct blur_params
	{
		float inv_image_size;
		float weights[k_max_num_weights];
		float radius;
	} blur_params;

	Texture2D source;
	Pipeline pipeline;
	ComputeShader shader;
	vk::descriptor_set descriptor_set;
};