#pragma once

#include "IRenderer.h"
#include "core/rendering/vulkan/VulkanUI.h"

static std::string env_map_folder("../../../data/textures/env/");
static VkFormat env_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;

struct ImageBasedLighting : public IRenderer
{
	void init() override
	{
		load_env_map("newport_loft.hdr");
		init_prefilter_diffuse();
		init_ui();
	}

	void load_env_map(const char* filename)
	{
		spherical_env_map.create_from_file(env_map_folder + filename, env_map_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	/* Pre-filtered diffuse environment map */
	void init_prefilter_diffuse()
	{
		/* Init pipeline */
		shader_prefilter_diffuse.create("fullscreen_quad_vert.vert.spv", "ibl_importance_sample_diffuse_frag.frag.spv");

		init_assets_prefilter_diffuse(prefiltered_env_map_render_size, false);
	}

	void init_assets_prefilter_diffuse(glm::ivec2 render_size, bool size_changed)
	{	
		if (size_changed)
		{
			vkDeviceWaitIdle(context.device);
			prefiltered_diffuse_env_map_attachment.destroy();
			ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(prefiltered_diffuse_env_map_ui_id));
		}

		prefiltered_env_map_render_size = render_size;
		prefiltered_diffuse_env_map_attachment.init(env_map_format, render_size.x, render_size.y, 1, 0, "Pre-filtered diffuse environment map attachment");
		prefiltered_diffuse_env_map_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		prefiltered_diffuse_env_map_attachment.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		prefiltered_diffuse_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, prefiltered_diffuse_env_map_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}

	void render_prefilter_diffuse()
	{
		/* 
			We want to solve the rendering equation, i.e find the outgoing radiance in the viewing direction, for a surface patch with normal N.
			We solve this by integrating over the contributions of all incoming radiances multiplied by the surface brdf, in the hemisphere over the surface patch. 
			We approximate this integral with importance sampling.
		*/
		VkCommandBuffer cmd_buf = begin_temp_cmd_buffer();
		VulkanRenderPassDynamic render_pass;
		render_pass.add_attachment(prefiltered_diffuse_env_map_attachment.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		prefiltered_diffuse_env_map_attachment.transition(cmd_buf, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		render_pass.begin(cmd_buf, prefiltered_env_map_render_size);
		vkCmdDraw(cmd_buf, 3, 1, 0, 0);
		render_pass.end(cmd_buf);
		prefiltered_diffuse_env_map_attachment.transition(cmd_buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

		end_temp_cmd_buffer(cmd_buf);
	}

	/* Pre-filtered specular environment map*/


	/* User Interface */
	void init_ui()
	{
		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		spherical_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, spherical_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}

	void show_ui()
	{
		if (ImGui::Begin("IBL Viewer"))
		{
			ImGui::SeparatorText("Spherical Environment Map");
			ImGui::Image(spherical_env_map_ui_id, ImGui::GetContentRegionAvail());

			ImGui::SeparatorText("Pre-filtered Environment Map");

			static glm::ivec2 render_size = prefiltered_env_map_render_size;

			ImGui::InputInt2("Render size", glm::value_ptr(prefiltered_env_map_render_size));
	
			ImGui::SameLine();
			if (ImGui::Button("Compute"))
			{
				if (render_size != prefiltered_env_map_render_size)
				{
					init_assets_prefilter_diffuse(prefiltered_env_map_render_size, true);
				}
				//render_prefilter_diffuse();
			}
			ImGui::Image(prefiltered_diffuse_env_map_ui_id, { (float)prefiltered_env_map_render_size.x, (float)prefiltered_env_map_render_size.y });
		}
		ImGui::End();
	}

	void create_pipeline() override {}
	bool reload_pipeline() override { return false; }
	void create_renderpass() override {}
	void render(VkCommandBuffer cmd_buffer) override {}


	/* 2D environment image storing the incoming radiances Li*/
	Texture2D spherical_env_map;
	ImTextureID spherical_env_map_ui_id;

	/* Stores for a given surface normal, the outgoing radiance. */
	glm::ivec2 prefiltered_env_map_render_size{ 256, 256 };
	Texture2D prefiltered_diffuse_env_map_attachment;
	ImTextureID prefiltered_diffuse_env_map_ui_id;
	Pipeline pipeline_diffuse_prefiltering;
	VertexFragmentShader shader_prefilter_diffuse;
};