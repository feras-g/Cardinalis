#pragma once

#include "IRenderer.h"

struct TwoPassGaussianBlur
{
	static constexpr int k_thread_group_size = 32;
	static constexpr int k_max_num_weights = 11;
	static constexpr const char* k_ps_range_name = "Blur Params";

	void init()
	{
		shader.create("two_pass_gaussian_blur_comp.comp.spv");

		descriptor_set.layout.add_storage_image_binding(0, "Source Image");
		descriptor_set.layout.add_storage_image_binding(1, "Intermediate Image");
		descriptor_set.layout.add_storage_image_binding(2, "Result Image");
		descriptor_set.layout.add_combined_image_sampler_binding(3, VK_SHADER_STAGE_COMPUTE_BIT, 1, "Scene Depth Texture");

		descriptor_set.layout.create("");
		descriptor_set.create("TwoPassGaussianBlur PostFX Descriptor Set");

		pipeline.layout.add_push_constant_range(k_ps_range_name, { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(blur_params) });

		VkDescriptorSetLayout descriptor_set_layouts[] = { descriptor_set.layout };
		pipeline.layout.create(descriptor_set_layouts);

		pipeline.create_compute(shader);
	}

	void set_source(const Texture2D& source)
	{
		render_size = { source.info.width, source.info.height };

		// Intermediate and Destination images with same properties as source
		intermediate.init(source.info.imageFormat, { source.info.width, source.info.height }, source.info.layerCount, false, "PostFX Blur Intermediate (Horizontal Pass)");
		intermediate.create(ctx.device, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		intermediate.transition_immediate(VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

		destination.init(source.info.imageFormat, { source.info.width, source.info.height }, source.info.layerCount, false, "PostFX Blur Destination");
		destination.create(ctx.device, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT); // VK_IMAGE_USAGE_SAMPLED_BIT because displayed in UI
		destination.transition_immediate(VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

		// Create UI image
		ui_texture_id_intermediate = ImGui_ImplVulkan_AddTexture(VulkanRendererCommon::get_instance().smp_clamp_nearest, intermediate.view, VK_IMAGE_LAYOUT_GENERAL);
		ui_texture_id_destination  = ImGui_ImplVulkan_AddTexture(VulkanRendererCommon::get_instance().smp_clamp_nearest, destination.view, VK_IMAGE_LAYOUT_GENERAL);

		descriptor_set.write_descriptor_storage_image(0, source.view);
		descriptor_set.write_descriptor_storage_image(1, intermediate.view);
		descriptor_set.write_descriptor_storage_image(2, destination.view);
		descriptor_set.write_descriptor_combined_image_sampler(3, DeferredRenderer::gbuffer[0].depth_attachment.view, VulkanRendererCommon::get_instance().smp_clamp_nearest);

		blur_params =
		{
			.thread_group_size = { k_thread_group_size, k_thread_group_size, 1 },
			.weights = { 0.227f, 0.194f, 0.121f, 0.0540f, 0.016f },
			.radius = 4,
			.pass = 0
		};
	}

	void execute(VkCommandBuffer cmd_buffer, const char* debug_marker_name)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, debug_marker_name);

		//const int num_workgroups = int(std::max(render_size.x, render_size.y) / k_thread_group_size);
		
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1, &descriptor_set.vk_set, 0, nullptr);
		pipeline.bind(cmd_buffer);

		// Horizontal blur
		blur_params.pass = 0;
		pipeline.cmd_push_constants(cmd_buffer, k_ps_range_name, &blur_params);
		vkCmdDispatch(cmd_buffer, num_workgroups.x, num_workgroups.y, num_workgroups.z);
		
		vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		// Vertical blur
		blur_params.pass = 1;
		pipeline.cmd_push_constants(cmd_buffer, k_ps_range_name, &blur_params);
		vkCmdDispatch(cmd_buffer, num_workgroups.x, num_workgroups.y, num_workgroups.z);
	}

	bool reload()
	{
		if (true == shader.compile())
		{
			if (true == pipeline.reload_pipeline())
			{
				return true;
			}
		}

		return false;
	}

	void show_ui()
	{
		ImGui::Begin("PostFX Blur");

		ImGui::Image(ui_texture_id_intermediate, { 1024, 1024 });
		ImGui::SameLine();
		ImGui::Image(ui_texture_id_destination, { 1024, 1024 });

		static bool shader_reload_success = true;
		if(ImGui::Button("Reload"))
		{
			shader_reload_success = reload();
		}

		if (shader_reload_success)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Reload success");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Reload failed");
		}

		ImGui::InputInt3("Workgroups", glm::value_ptr(num_workgroups));
		ImGui::InputInt("Radius", &blur_params.radius);


		ImGui::End();
	}

	struct blur_params
	{
		glm::ivec3 thread_group_size;
		float weights[k_max_num_weights];
		int radius;
		int pass; // 0: horizontal blur , 1: vertical blur
	} blur_params;

	glm::vec2 render_size;

	Texture2D intermediate;
	Texture2D destination;

	glm::ivec3 num_workgroups{ 32, 32, 1 };

	Pipeline pipeline;
	ComputeShader shader;
	vk::descriptor_set descriptor_set;

	ImTextureID ui_texture_id_intermediate;
	ImTextureID ui_texture_id_destination;
};