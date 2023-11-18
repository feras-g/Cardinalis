#pragma once

#include "IRenderer.h"
#include "core/rendering/vulkan/VulkanUI.h"

static std::string env_map_folder("../../../data/textures/env/");
static VkFormat env_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;

struct PrefilteredDiffuseEnvMap
{
	/* Pre-filtered diffuse environment map */
	void init(Texture2D& spherical_env_map)
	{
		init_assets(attachment_render_size, false);
		init_pipeline(spherical_env_map);
	}

	void init_pipeline(Texture2D& spherical_env_map)
	{
		shader.create("fullscreen_quad_vert.vert.spv", "ibl_importance_sample_diffuse_frag.frag.spv");

		std::array<VkDescriptorPoolSize, 1> pool_sizes
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &VulkanRendererCommon::get_instance().s_SamplerClampNearest, "Spherical env map");
		descriptor_set.layout.add_uniform_buffer_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, "Diffuse Env Map Prefiltering parameters");
		descriptor_set.layout.create("Diffuse Env Map Prefiltering Shader Params Layout");
		descriptor_set.create(descriptor_pool, "Diffuse Env Map Prefiltering");

		descriptor_set.write_descriptor_combined_image_sampler(0, spherical_env_map.view, VulkanRendererCommon::get_instance().s_SamplerClampNearest);
		descriptor_set.write_descriptor_storage_buffer(1, ubo_shader_params, 0, VK_WHOLE_SIZE);

		VkDescriptorSetLayout layouts[]
		{
			descriptor_set.layout
		};


		pipeline.layout.create(layouts);

		VkFormat color_format[]
		{
			env_map_format
		};

		pipeline.create_graphics(shader, color_format, VK_FORMAT_UNDEFINED, Pipeline::Flags::NONE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void init_assets(glm::ivec2 render_size, bool size_changed)
	{
		if (size_changed)
		{
			vkDeviceWaitIdle(context.device);
			result_attachment.destroy();
			ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(prefiltered_diffuse_env_map_ui_id));
			render_pass.reset();
		}

		attachment_render_size = render_size;
		result_attachment.init(env_map_format, attachment_render_size.x, attachment_render_size.y, 1, 0, "Pre-filtered diffuse environment map attachment");
		result_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		result_attachment.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		render_pass.add_attachment(result_attachment.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		prefiltered_diffuse_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, result_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		/* Uniform buffer */
		ubo_shader_params.init(Buffer::Type::UNIFORM, sizeof(ShaderParams), "Diffuse Env Map Shader Params");
		ubo_shader_params.create();
		update_shader_params_ubo();
	}

	void update_shader_params_ubo()
	{
		ubo_shader_params.upload(context.device, &shader_params, 0, sizeof(ShaderParams));
	}

	void render()
	{
		/*
			We want to solve the rendering equation, i.e find the outgoing radiance in the viewing direction, for a surface patch with normal N.
			We solve this by integrating over the contributions of all incoming radiances multiplied by the surface brdf, in the hemisphere over the surface patch.
			We approximate this integral with importance sampling.
		*/
		VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
		set_viewport_scissor(cmd_buffer, attachment_render_size.x, attachment_render_size.y, false);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptor_set.vk_set, 0, nullptr);
		result_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		render_pass.begin(cmd_buffer, attachment_render_size);
		vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
		render_pass.end(cmd_buffer);
		result_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		end_temp_cmd_buffer(cmd_buffer);
	}

	void show_ui()
	{
		ImGui::SeparatorText("Pre-filtered Environment Map");

		static glm::ivec2 input_render_size = attachment_render_size;
		static int input_num_samples = shader_params.num_samples;

		ImGui::SliderInt2("Render size", glm::value_ptr(input_render_size), 0, 4096);
		ImGui::SliderInt("# Samples", &input_num_samples, 0, 4096);
		if (ImGui::Button("Render"))
		{
			if (input_render_size != attachment_render_size)
			{
				init_assets(input_render_size, true);
			}

			if (input_num_samples != shader_params.num_samples)
			{
				shader_params.num_samples = std::max(input_num_samples, 0);
				update_shader_params_ubo();
			}

			render();
		}
		ImGui::Image(prefiltered_diffuse_env_map_ui_id, { (float)result_attachment.info.width, (float)result_attachment.info.height });
	}


	struct ShaderParams
	{
		int env_map_width = 1024;
		int env_map_height = 1024;
		unsigned int num_samples = 256; /* Number of samples used for importance sampling. Default: 256. */
	} shader_params;
	
	glm::ivec2 attachment_render_size{ 256, 256 };
	Texture2D result_attachment;	/* Stores for a given surface normal, the outgoing radiance. */
	ImTextureID prefiltered_diffuse_env_map_ui_id;
	Pipeline pipeline;
	VertexFragmentShader shader;
	VulkanRenderPassDynamic render_pass;
	DescriptorSet descriptor_set;
	VkDescriptorPool descriptor_pool;
	Buffer ubo_shader_params;
};



struct ImageBasedLighting : public IRenderer
{
	void init() override
	{
		init_assets("newport_loft.hdr");
		prefiltered_diffuse.init(spherical_env_map);
	}

	void init_assets(const char* filename)
	{
		spherical_env_map.create_from_file(env_map_folder + filename, env_map_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		spherical_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, spherical_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}

	void show_ui()
	{
		if (ImGui::Begin("IBL Viewer"))
		{
			ImGui::SeparatorText("Spherical Environment Map");
			ImGui::Image(spherical_env_map_ui_id, ImGui::GetContentRegionAvail());
			prefiltered_diffuse.show_ui();
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

	PrefilteredDiffuseEnvMap prefiltered_diffuse;
};


