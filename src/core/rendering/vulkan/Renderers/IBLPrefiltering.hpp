#pragma once

#include "core/rendering/vulkan/Renderers/CubemapRenderer.hpp"

static const std::string env_map_folder("../../../data/textures/env/");
static constexpr VkFormat env_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;
//static constexpr VkFormat env_map_format = VK_FORMAT_R8G8B8A8_UNORM;

// Real Shading in Unreal Engine 4, Presentations Notes, page 6 (https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf)
static VkFormat brdf_integration_map_format = VK_FORMAT_R16G16_UNORM;

static CubemapRenderer cubemap_renderer;

struct IBLRenderer
{
	VkSampler sampler_clamp_linear;
	VkSampler sampler_clamp_nearest;
	VkSampler sampler_repeat_nearest;
	VkSampler sampler_repeat_linear;

	/* Pre-filtered diffuse environment map */
	void init(const char* env_map_filename)
	{
		sampler_clamp_linear = VulkanRendererCommon::get_instance().s_SamplerClampLinear;
		sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		sampler_repeat_nearest = VulkanRendererCommon::get_instance().s_SamplerRepeatNearest;
		sampler_repeat_linear = VulkanRendererCommon::get_instance().s_SamplerRepeatLinear;

		create_env_map(env_map_filename);
		init_assets(false);
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
		descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Spherical env map");
		descriptor_set.layout.add_uniform_buffer_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, "Diffuse Env Map Prefiltering parameters");
		descriptor_set.layout.create("Diffuse Env Map Prefiltering Shader Params Layout");
		descriptor_set.create(descriptor_pool, "Diffuse Env Map Prefiltering");
		descriptor_set.write_descriptor_combined_image_sampler(0, spherical_env_map.view, sampler_clamp_nearest);
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

		pipeline.create_graphics(shader, color_format, VK_FORMAT_UNDEFINED, Pipeline::Flags::NONE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		VkPipelineLayout empty_layout = {};
		VkPipelineLayoutCreateInfo empty_info = {};
		vkCreatePipelineLayout(context.device, &empty_info, nullptr, &empty_layout);
		shader_brdf_integration.create("fullscreen_quad_vert.vert.spv", "integrate_brdf_frag.frag.spv");
		pipeline_brdf_integration.create_graphics(shader_brdf_integration, std::span<VkFormat>(&brdf_integration_map_format, 1), VK_FORMAT_UNDEFINED, Pipeline::Flags::NONE, empty_layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void init_assets(bool size_changed)
	{
		if (size_changed)
		{
			vkDeviceWaitIdle(context.device);
			prefiltered_diffuse_env_map.destroy();
			ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(prefiltered_diffuse_env_map_ui_id));
			render_pass.reset();
		}

		glm::vec2 spherical_env_map_size = { spherical_env_map.info.width, spherical_env_map.info.height };

		/* Diffuse env map prefiltering */
		{
			prefiltered_diffuse_env_map.init(env_map_format, spherical_env_map_size, 1, false, "Pre-filtered diffuse environment map attachment");
			prefiltered_diffuse_env_map.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			prefiltered_diffuse_env_map.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
			prefiltered_diffuse_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, prefiltered_diffuse_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		/* Specular prefiltering render */
		{
			prefiltered_specular_env_map.init(env_map_format, spherical_env_map_size, 1, false, "Pre-filtered specular environment map attachment");
			/* 6 mip levels for each roughness increment : 0.0, 0.2, 0.4, 0.8, 1.0 */
			prefiltered_specular_env_map.info.mipLevels = k_specular_mip_levels;
			prefiltered_specular_env_map.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			prefiltered_specular_env_map.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

			glm::vec2 mip_size = spherical_env_map_size;
			for (int mip = 0; mip < k_specular_mip_levels; mip++)
			{
				Texture2D::create_texture_2d_mip_view(specular_mip_views[mip], prefiltered_specular_env_map, context.device, mip);
				prefiltered_specular_env_map_ui_id[mip] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, specular_mip_views[mip], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
				specular_mip_sizes[mip] = mip_size;
				mip_size = { mip_size.x / 2, mip_size.y / 2 };
			}
		}

		/* Environment BRDF LUT */
		{
			brdf_integration_map.init(brdf_integration_map_format, brdf_integration_map_size, brdf_integration_map_size, 1, false, "IBL Brdf integration map");
			brdf_integration_map.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			brdf_integration_map.transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
			brdf_integration_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, brdf_integration_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}
	}

	void create_env_map(const char* filename)
	{
		/* Source spherical env map */
		spherical_env_map.create_from_file(env_map_folder + filename, env_map_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		spherical_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, spherical_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		/* Create descriptor set for the env map */
		VkDescriptorPoolSize pool_sizes[1]
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		spherical_env_map_descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "");
		spherical_env_map_descriptor_set.layout.create("");
		spherical_env_map_descriptor_set.create(descriptor_pool, "Spherical Env map descriptor set");

		spherical_env_map_descriptor_set.write_descriptor_combined_image_sampler(0, spherical_env_map.view, sampler_clamp_nearest);
	}

	void init_ubo()
	{
		ubo_shader_params.init(Buffer::Type::UNIFORM, sizeof(ShaderParams), "Diffuse Env Map Shader Params");
		ubo_shader_params.create();

		/* Defaults */
		shader_params.k_env_map_width = spherical_env_map.info.width;
		shader_params.k_env_map_height = spherical_env_map.info.height;
		shader_params.num_samples_diffuse = 4096;
		shader_params.base_mip_diffuse = 2;

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
		pipeline.bind(cmd_buffer);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptor_set.vk_set, 0, nullptr);


		/* Diffuse */
		if (shader_params.mode == MODE_PREFILTER_DIFFUSE)
		{
			int placeholder_val = 0;
			pipeline.layout.cmd_push_constants(cmd_buffer, "Specular Mip Level", &placeholder_val);
			set_viewport_scissor(cmd_buffer, spherical_env_map.info.width, spherical_env_map.info.height, true);
			prefiltered_diffuse_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			render_pass.reset();
			render_pass.add_color_attachment(prefiltered_diffuse_env_map.view);
			render_pass.begin(cmd_buffer, { spherical_env_map.info.width, spherical_env_map.info.height });
			vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
			render_pass.end(cmd_buffer);
			prefiltered_diffuse_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		}
		else if (shader_params.mode == MODE_PREFILTER_SPECULAR)
		{
			/* Specular */
			prefiltered_specular_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			for (int mip = 0; mip < k_specular_mip_levels; mip++)
			{
				pipeline.layout.cmd_push_constants(cmd_buffer, "Specular Mip Level", &mip);
				set_viewport_scissor(cmd_buffer, specular_mip_sizes[mip].x, specular_mip_sizes[mip].y, true);
				render_pass.reset();
				render_pass.add_color_attachment(specular_mip_views[mip]);
				render_pass.begin(cmd_buffer, specular_mip_sizes[mip]);
				vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
				render_pass.end(cmd_buffer);
			}
			prefiltered_specular_env_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		}
		else if (shader_params.mode == MODE_BRDF_INTEGRATION)
		{
			pipeline_brdf_integration.bind(cmd_buffer);
			set_viewport_scissor(cmd_buffer, brdf_integration_map_size, brdf_integration_map_size, true);
			brdf_integration_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			render_pass.reset();
			render_pass.add_color_attachment(brdf_integration_map.view);
			render_pass.begin(cmd_buffer, { brdf_integration_map_size, brdf_integration_map_size });
			vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
			render_pass.end(cmd_buffer);
			brdf_integration_map.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		}

		end_temp_cmd_buffer(cmd_buffer);
	}

	void show_ui()
	{
		const ImVec2 thumbnail_size = { 512, 256 };

		if (ImGui::Begin("IBL Viewer"))
		{
			/*************************************************************************************************/
			ImGui::SeparatorText("Spherical Environment Map");
			ImGui::Image(spherical_env_map_ui_id, thumbnail_size);

			/*************************************************************************************************/
			ImGui::SeparatorText("Pre-filtered Diffuse Environment Map");
			ImGui::Image(prefiltered_diffuse_env_map_ui_id, thumbnail_size);

			if (ImGui::Button("Render Diffuse"))
			{
				shader_params.mode = Mode::MODE_PREFILTER_DIFFUSE;
				update_shader_params_ubo();
				render();
			}
			
			ImGui::SeparatorText("Sample count");
			ImGui::InputScalar("##Sample count", ImGuiDataType_U32, &shader_params.num_samples_diffuse);
			ImGui::SeparatorText("Mipmap level");
			ImGui::InputScalar("##Mipmap level", ImGuiDataType_U32, &shader_params.base_mip_diffuse);

			/*************************************************************************************************/
			ImGui::SeparatorText("Pre-filtered Specular Environment Map");

			for (int mip = 0; mip < k_specular_mip_levels; mip++)
			{
				ImGui::Image(prefiltered_specular_env_map_ui_id[mip], { float(int(thumbnail_size.x) >> mip) ,  float(int(thumbnail_size.y) >> mip) });
				if (mip < k_specular_mip_levels - 1)
				{
					ImGui::SameLine();
				}
			}
			if (ImGui::Button("Render Specular"))
			{
				shader_params.mode = Mode::MODE_PREFILTER_SPECULAR;
				update_shader_params_ubo();
				render();
			}

			/*************************************************************************************************/
			ImGui::SeparatorText("BRDF Integration");

			ImGui::Image(brdf_integration_map_ui_id, { 256, 256 });
			if (ImGui::Button("Render"))
			{
				shader_params.mode = Mode::MODE_BRDF_INTEGRATION;
				update_shader_params_ubo();
				render();
			}

			// Begin a vertical panel for buttons and inputs
			ImGui::BeginGroup();

			// Display input fields

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

		if (shader.compile() && shader_brdf_integration.compile())
		{
			pipeline.reload_pipeline();
			pipeline_brdf_integration.reload_pipeline();
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
		unsigned int k_env_map_width;		// Width of source environment map.
		unsigned int k_env_map_height;		// Height of source environment map.
		unsigned int num_samples_diffuse;	// Number of num_samples_diffuse for importance sampling.
		unsigned int base_mip_diffuse;		// Base mipmap level of the environment map to sample from.
		unsigned int mode = 0;				// Whether to prefilter diffuse or specular.
	} shader_params;

	enum Mode
	{
		MODE_PREFILTER_DIFFUSE = 0,
		MODE_PREFILTER_SPECULAR = 1,
		MODE_BRDF_INTEGRATION = 2,
	};

	/* Diffuse environment map prefiltering */
	static inline Texture2D prefiltered_diffuse_env_map;	/* Stores for a given surface normal, the outgoing radiance. */

	/* Specular environment map prefiltering */
	static inline Texture2D prefiltered_specular_env_map;	
	static constexpr int k_specular_mip_levels = 6;
	VkImageView specular_mip_views[k_specular_mip_levels];
	glm::vec2 specular_mip_sizes[k_specular_mip_levels];

	/* User Interface */
	ImTextureID prefiltered_diffuse_env_map_ui_id;
	ImTextureID prefiltered_specular_env_map_ui_id[k_specular_mip_levels];
	ImTextureID brdf_integration_map_ui_id;

	/* BRDF Integration map */
	VertexFragmentShader shader_brdf_integration;
	static inline Texture2D brdf_integration_map;
	Pipeline pipeline_brdf_integration;
	int brdf_integration_map_size = 512;

	/* Common */
	VulkanRenderPassDynamic render_pass;
	Pipeline pipeline;
	VertexFragmentShader shader;
	DescriptorSet descriptor_set;
	VkDescriptorPool descriptor_pool;
	Buffer ubo_shader_params;

	static inline bool is_initialized = false;
};