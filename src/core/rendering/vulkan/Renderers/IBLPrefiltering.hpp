#pragma once

#include "core/rendering/vulkan/Renderers/CubemapRenderer.hpp"

static const std::string env_map_folder("../../../data/textures/env/");
static constexpr VkFormat env_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;
//static constexpr VkFormat env_map_format = VK_FORMAT_R8G8B8A8_UNORM;

static CubemapRenderer cubemap_renderer;

struct IBLRenderer
{
	/* Pre-filtered diffuse environment map */
	void init(const char* env_map_filename)
	{
		create_env_map(env_map_filename);
		init_assets(attachment_render_size, false);
		init_ubo();
		init_pipeline(spherical_env_map);
		render();
		is_initialized = true;

		cubemap_renderer.init(spherical_env_map);
		cubemap_renderer.render();
	}

	void init_pipeline(Texture2D& spherical_env_map)
	{
		VkDescriptorPoolSize pool_sizes[2]
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		shader.create("fullscreen_quad_vert.vert.spv", "importance_sample_diffuse_frag.frag.spv");

		/* Init descriptor set for prefiltered maps rendering */
		descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &VulkanRendererCommon::get_instance().s_SamplerClampNearest, "Spherical env map");
		descriptor_set.layout.add_uniform_buffer_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, "Diffuse Env Map Prefiltering parameters");
		descriptor_set.layout.create("Diffuse Env Map Prefiltering Shader Params Layout");
		descriptor_set.create(descriptor_pool, "Diffuse Env Map Prefiltering");
		descriptor_set.write_descriptor_combined_image_sampler(0, spherical_env_map.view, VulkanRendererCommon::get_instance().s_SamplerClampNearest);
		descriptor_set.write_descriptor_uniform_buffer(1, ubo_shader_params, 0, VK_WHOLE_SIZE);

		VkDescriptorSetLayout layouts[]
		{
			descriptor_set.layout
		};

		pipeline.layout.add_push_constant_range("Specular Mip Level", { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(unsigned int) });
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
			prefiltered_diffuse_env_map.destroy();
			ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(prefiltered_diffuse_env_map_ui_id));
			render_pass_prefilter_diffuse.reset();
		}

		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;

		/* Diffuse prefiltering render */
		attachment_render_size = render_size;
		prefiltered_diffuse_env_map.init(env_map_format, attachment_render_size.x, attachment_render_size.y, 1, false, "Pre-filtered diffuse environment map attachment");
		prefiltered_diffuse_env_map.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		prefiltered_diffuse_env_map.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		render_pass_prefilter_diffuse.add_attachment(prefiltered_diffuse_env_map.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		prefiltered_diffuse_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, prefiltered_diffuse_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		/* Specular prefiltering render */
		attachment_render_size = render_size;
		prefiltered_specular_env_map.init(env_map_format, attachment_render_size.x, attachment_render_size.y, 1, false, "Pre-filtered specular environment map attachment");
		/* 6 mip levels for each roughness increment : 0.0, 0.2, 0.4, 0.8, 1.0 */
		prefiltered_specular_env_map.info.mipLevels = k_specular_mip_levels; 
		prefiltered_specular_env_map.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		prefiltered_specular_env_map.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

		glm::vec2 mip_size = attachment_render_size;
		for (int mip = 0; mip < k_specular_mip_levels; mip++)
		{
			Texture2D::create_texture_2d_mip_view(specular_mip_views[mip], prefiltered_specular_env_map, context.device, mip);
			render_pass_prefilter_specular.add_color_attachment(specular_mip_views[mip]);
			prefiltered_specular_env_map_ui_id[mip] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, specular_mip_views[mip], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			specular_mip_sizes[mip] = mip_size;
			mip_size = { mip_size.x / 2, mip_size.y / 2 };
		}
	}

	void create_env_map(const char* filename)
	{
		/* Source spherical env map */
		spherical_env_map.create_from_file(env_map_folder + filename, env_map_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		spherical_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, spherical_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		/* Create descriptor set for the env map */
		VkDescriptorPoolSize pool_sizes[1]
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		spherical_env_map_descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &VulkanRendererCommon::get_instance().s_SamplerClampLinear, "");
		spherical_env_map_descriptor_set.layout.create("");
		spherical_env_map_descriptor_set.create(descriptor_pool, "Spherical Env map descriptor set");

		VkSampler& sampler_repeat_linear = VulkanRendererCommon::get_instance().s_SamplerRepeatLinear;
		spherical_env_map_descriptor_set.write_descriptor_combined_image_sampler(0, spherical_env_map.view, sampler_repeat_linear);
	}

	void init_ubo()
	{
		ubo_shader_params.init(Buffer::Type::UNIFORM, sizeof(ShaderParams), "Diffuse Env Map Shader Params");
		ubo_shader_params.create();
		update_shader_params_ubo();
	}

	void update_shader_params_ubo()
	{
		ubo_shader_params.upload(context.device, &shader_params, 0, sizeof(ShaderParams));
	}

	/* Renders the prefiltered diffuse env map */
	void render()
	{
		/*
			We want to solve the rendering equation, i.e find the outgoing radiance in the viewing direction give an incoming light direction.
			We solve this by integrating over the contributions of all incoming radiances multiplied by the surface brdf, in the hemisphere over the surface patch.
			We approximate this integral with importance sampling.
		*/
		VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptor_set.vk_set, 0, nullptr);

		
		/* Diffuse */
		if (shader_params.prefilter_diffuse)
		{
			int placeholder_val = 0;
			pipeline.layout.cmd_push_constants(cmd_buffer, "Specular Mip Level", &placeholder_val);
			set_viewport_scissor(cmd_buffer, attachment_render_size.x, attachment_render_size.y, false);
			prefiltered_diffuse_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			render_pass_prefilter_diffuse.begin(cmd_buffer, attachment_render_size);
			vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
			render_pass_prefilter_diffuse.end(cmd_buffer);
			prefiltered_diffuse_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		}
		else
		{
			/* Specular */
			prefiltered_specular_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			for (int mip = 0; mip < k_specular_mip_levels; mip++)
			{
				pipeline.layout.cmd_push_constants(cmd_buffer, "Specular Mip Level", &mip);
				set_viewport_scissor(cmd_buffer, specular_mip_sizes[mip].x, specular_mip_sizes[mip].y, false);
				render_pass_prefilter_specular.reset();
				render_pass_prefilter_specular.add_color_attachment(specular_mip_views[mip]);
				render_pass_prefilter_specular.begin(cmd_buffer, specular_mip_sizes[mip]);
				vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
				render_pass_prefilter_specular.end(cmd_buffer);

			}
			prefiltered_specular_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		}

		end_temp_cmd_buffer(cmd_buffer);
	}

	void show_ui()
	{
		if (ImGui::Begin("IBL Viewer"))
		{
			ImGui::SeparatorText("Spherical Environment Map");
			ImGui::Image(spherical_env_map_ui_id, { (float)spherical_env_map.info.width, (float)spherical_env_map.info.height });

			ImGui::SeparatorText("Pre-filtered Diffuse Environment Map");
			ImGui::Image(prefiltered_diffuse_env_map_ui_id, { (float)prefiltered_diffuse_env_map.info.width, (float)prefiltered_diffuse_env_map.info.height });

			ImGui::SeparatorText("Pre-filtered Specular Environment Map");
			for (int mip = 0; mip < k_specular_mip_levels; mip++)
			{
				ImGui::Image(prefiltered_specular_env_map_ui_id[mip], { (float)specular_mip_sizes[mip].x,  specular_mip_sizes[mip].y });
				if (mip < k_specular_mip_levels - 1)
				{
					ImGui::SameLine();
				}
			}

			// Begin a vertical panel for buttons and inputs
			ImGui::BeginGroup();

			// Display input fields
			static glm::ivec2 input_render_size = attachment_render_size;
			static unsigned int input_num_samples = shader_params.num_samples;
			static unsigned int input_mipmap_level = shader_params.mipmap_level;
			static bool input_prefilter_diffuse = shader_params.prefilter_diffuse;

			ImGui::SeparatorText("Render size");
			ImGui::InputInt2("##Render size", glm::value_ptr(input_render_size));
			if (input_prefilter_diffuse)
			{
				ImGui::SeparatorText("Sample count");
				ImGui::InputScalar("##Sample count", ImGuiDataType_U32, &input_num_samples);
				ImGui::SeparatorText("Mipmap level");
				ImGui::InputScalar("##Mipmap level", ImGuiDataType_U32, &input_mipmap_level);
			}

			ImGui::SeparatorText("Prefilter mode");
			const char* active_modes[2] = { "Specular", "Diffuse" };
			ImGui::Text("Active mode: %s", active_modes[input_prefilter_diffuse]);

			if (ImGui::Button("Diffuse"))
			{
				input_prefilter_diffuse = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Specular"))
			{
				input_prefilter_diffuse = false;
			}

			// Display buttons
			if (ImGui::Button("Render"))
			{
				bool dirty_params = false;

				if (input_render_size != attachment_render_size)
				{
					init_assets(input_render_size, true);
				}

				if (input_num_samples != shader_params.num_samples)
				{
					shader_params.num_samples = input_num_samples;
					dirty_params = true;
				}

				if (input_mipmap_level != shader_params.mipmap_level)
				{
					shader_params.mipmap_level = input_mipmap_level;
					dirty_params = true;
				}

				if (input_prefilter_diffuse != shader_params.prefilter_diffuse)
				{
					shader_params.prefilter_diffuse = input_prefilter_diffuse;
					dirty_params = true;
				}

				if (dirty_params)
				{
					update_shader_params_ubo();
				}

				render();
			}
			ImGui::SameLine();

			if (ImGui::Button("Reload pipeline"))
			{
				if (reload_pipeline())
				{
					render();
					ImGui::Text("Reload success");
				}
			}

			// End the vertical panel
			ImGui::EndGroup();
		}
		ImGui::End();
	}

	bool reload_pipeline()
	{
		vkDeviceWaitIdle(context.device);

		if (shader.compile())
		{
			pipeline.reload_pipeline();
			return true;
		}

		return false;
	}

	/* 2D environment image storing the incoming radiances Li */
	Texture2D spherical_env_map;
	ImTextureID spherical_env_map_ui_id;
	DescriptorSet spherical_env_map_descriptor_set;

	struct ShaderParams
	{
		unsigned int k_env_map_width = 1024;	/* Env map original size, do not modify. */
		unsigned int k_env_map_height = 512;
		unsigned int num_samples = 512;			/* Number of samples used for importance sampling. Default: 256.*/
		unsigned int mipmap_level = 5;			/* Mipmap level of env map to sample values from.  Default: 0*/
		bool prefilter_diffuse = true;			/* Wether to prefilter diffuse or specular. Default: true. */
	} shader_params;

	glm::ivec2 attachment_render_size{ shader_params.k_env_map_width, shader_params.k_env_map_height };

	VulkanRenderPassDynamic render_pass_prefilter_diffuse;
	static inline Texture2D prefiltered_diffuse_env_map;	/* Stores for a given surface normal, the outgoing radiance. */
	ImTextureID prefiltered_diffuse_env_map_ui_id;

	VulkanRenderPassDynamic render_pass_prefilter_specular;
	static inline Texture2D prefiltered_specular_env_map;	
	static constexpr int k_specular_mip_levels = 6;
	ImTextureID prefiltered_specular_env_map_ui_id[k_specular_mip_levels];
	VkImageView specular_mip_views[k_specular_mip_levels];
	glm::vec2 specular_mip_sizes[k_specular_mip_levels];

	struct SpecularMipFilteringValues
	{
		unsigned int samples;
		float roughness;
	};

	Pipeline pipeline;
	VertexFragmentShader shader;
	DescriptorSet descriptor_set;
	VkDescriptorPool descriptor_pool;
	Buffer ubo_shader_params;

	static inline bool is_initialized = false;
};