#pragma once

#include "IRenderer.h"
#include "core/rendering/vulkan/RenderObjectManager.h"
#include "core/rendering/camera.h"
#include "core/rendering/vulkan/VulkanMesh.h"

struct ShadowRenderer : public IRenderer
{
	static constexpr VkFormat k_depth_format = VK_FORMAT_D32_SFLOAT;
	static constexpr uint32_t k_depth_size = 2048;
	static constexpr unsigned k_num_cascades = 4;
	static constexpr uint32_t view_mask = 0b00001111;

	void init() override
	{
		name = "Shadow Renderer";
		draw_metrics = DrawMetricsManager::add_entry(name.c_str());

		create_resources();
		create_renderpass();
		create_pipeline();

		is_initialized = true;
	}

	void create_resources()
	{
		for (int frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
		{
			// Buffers
			cascades_data[frame_idx].num_cascades = k_num_cascades;
			ssbo_cascades_data[frame_idx].init(vk::buffer::type::STORAGE, sizeof(CascadesData), "Shadow Renderer: Cascades Data");
			ssbo_cascades_data[frame_idx].create();
			// Textures
			shadow_cascades_depth[frame_idx].init(k_depth_format, { k_depth_size, k_depth_size }, k_num_cascades, false, "Shadow Maps Array");
			shadow_cascades_depth[frame_idx].info.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			shadow_cascades_depth[frame_idx].create(ctx.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			shadow_cascades_depth[frame_idx].view = Texture2D::create_texture_2d_array_view(shadow_cascades_depth[frame_idx], k_depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

			// Texture views
			for (int layer = 0; layer < k_num_cascades; layer++)
			{
				shadow_cascades_depth[frame_idx].create_texture_2d_layer_view(shadow_cascades_view[frame_idx][layer], shadow_cascades_depth[frame_idx], ctx.device, layer);
				shadow_cascades_view_ui_id[frame_idx][layer] = ImGui_ImplVulkan_AddTexture(VulkanRendererCommon::get_instance().s_SamplerRepeatNearest, shadow_cascades_view[frame_idx][layer], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	}

	void create_pipeline() override
	{
		// Shader
		shader.create("cascaded_shadow_transform_vert.vert.spv", "output_fragment_depth_frag.frag.spv");

		// Descriptor Set
		VkDescriptorPoolSize pool_sizes[]
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

		descriptor_set_layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "Shadow Cascade SSBO");
		descriptor_set_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Shadow Cascades Texture Array");
		descriptor_set_layout.create("Shadow Cascade Descriptor Set Layout");

		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;

		for (int frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
		{
			descriptor_set[frame_idx].assign_layout(descriptor_set_layout);
			descriptor_set[frame_idx].create(descriptor_pool, "Shadow Renderer Descriptor Set");
			descriptor_set[frame_idx].write_descriptor_storage_buffer(0, ssbo_cascades_data[frame_idx], 0, VK_WHOLE_SIZE);
			descriptor_set[frame_idx].write_descriptor_combined_image_sampler(1, shadow_cascades_depth[frame_idx].view, sampler_clamp_nearest);
		}

		// Pipeline
		pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });

		VkDescriptorSetLayout layouts[] { ObjectManager::get_instance().mesh_descriptor_set_layout, descriptor_set_layout };
		pipeline.layout.create(layouts);
		pipeline.create_graphics(shader, {}, k_depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, view_mask);
	}

	void create_renderpass() override
	{
		for (int frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
		{
			renderpass[frame_idx].add_depth_attachment(shadow_cascades_depth[frame_idx].view);
		}
	}

	void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list, camera& camera, const VulkanRendererCommon::FrameData frame_data, glm::vec4 directional_light_dir)
	{
		//updateCascades(camera, frame_data, directional_light_dir);

		compute_cascade_splits(camera.znear, camera.zfar, lambda);
		compute_cascade_projection(camera, directional_light_dir);

		set_viewport_scissor(cmd_buffer, k_depth_size, k_depth_size, true);

		ObjectManager& object_manager = ObjectManager::get_instance();

		VkDescriptorSet bound_descriptor_sets[] = { descriptor_set[ctx.curr_frame_idx] };

		pipeline.bind(cmd_buffer);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, bound_descriptor_sets, 0, nullptr);

		shadow_cascades_depth[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
		renderpass[ctx.curr_frame_idx].begin(cmd_buffer, { k_depth_size, k_depth_size }, view_mask);
		object_manager.draw_mesh_list(cmd_buffer, mesh_list, pipeline.layout, bound_descriptor_sets, draw_metrics);
		renderpass[ctx.curr_frame_idx].end(cmd_buffer);
		shadow_cascades_depth[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	}

	void render(VkCommandBuffer cmd_buffer) override
	{

	}

	void on_window_resize()
	{
		create_renderpass();
	}

	virtual bool reload_pipeline()
	{
		return false;
	}

	
	/*
		z_near			Camera near clip.
		z_far			Camera far clip.
		lambda			Interpolation factor controlling the mix between uniform and logarithmic distribution for cascade splits.
	*/
	void compute_cascade_splits(float z_near, float z_far, float lambda)
	{
		// Use Practical Split Scheme method
		// https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf, page 6
		float z_range = z_far - z_near;
		float z_min = z_near;
		float z_max = z_near + z_range;
		float z_ratio = z_max / z_min;

		for (int i = 0; i < k_num_cascades; ++i)
		{
			float p = (i + 1) / (float)(k_num_cascades);
			float log = z_min * std::pow(z_ratio, p);
			float uniform = z_min + z_range * p;
			float z = lambda * (log - uniform) + uniform;

			z_splits[i] = (z - z_near) / z_range;
		}
	}

	void compute_cascade_projection(camera& camera, glm::vec4 directional_light_dir)
	{
		/* Light direction */
		glm::vec3 L = glm::normalize(-directional_light_dir);

		float distance_prev_cascade = 0.0f;
		for (int c = 0; c < k_num_cascades; c++)
		{
			/* Frustum corners in Normalized Device Coordinates (= Cube) */
			glm::vec3 frustum_corners[8] =
			{
				glm::vec3(-1.0f,  1.0f, 0.0f),
				glm::vec3( 1.0f,  1.0f, 0.0f),
				glm::vec3( 1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f,  1.0f, 1.0f),
				glm::vec3( 1.0f,  1.0f, 1.0f),
				glm::vec3( 1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f),
			};

			glm::mat4 inv_view_projection = glm::inverse(camera.projection * camera.view);

			/* Frustum corners in World Space (= Truncated pyramid) */
			for (int corner_idx = 0; corner_idx < 8; corner_idx++)
			{
				glm::vec4 corner = inv_view_projection * glm::vec4(frustum_corners[corner_idx], 1.0f);
				frustum_corners[corner_idx] = corner / corner.w;
			}

			/* 
				To get the 4 corners corresponding to the current cascade,
				we need to scale the original world space frustum corners
				by the stored previous and current cascade distances from the near-plane.
			*/
			float distance_prev_cascade = c == 0 ? 0.0f : z_splits[c - 1];
			float distance_curr_cascade = z_splits[c];


			for (int corner_idx = 0; corner_idx < 4; corner_idx++)
			{
				glm::vec3 r = frustum_corners[corner_idx + 4] - frustum_corners[corner_idx];
				frustum_corners[corner_idx + 4] = frustum_corners[corner_idx] + (r * distance_curr_cascade);
				frustum_corners[corner_idx] = frustum_corners[corner_idx] + (r * distance_prev_cascade);
			}

			/* Cascade center */
			glm::vec3 center = glm::vec3(0);
			for (int corner_idx = 0; corner_idx < 8; corner_idx++)
			{
				center += frustum_corners[corner_idx];
			}
			center /= 8.0f;

			/* */
			const glm::vec3 up = camera.right;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) 
			{
				float distance = glm::length(frustum_corners[i] - center);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			const glm::vec3 aabb_max{  radius,  radius,  radius };
			const glm::vec3 aabb_min = -aabb_max;

			glm::mat4 cascade_proj = glm::orthoRH_ZO(aabb_min.x, aabb_max.x, aabb_min.y, aabb_max.y, 0.0f, (aabb_max.z - aabb_min.z));	// /!\ VERY important to have the correct handedness !!!
			glm::mat4 cascade_view = glm::lookAtRH(center + L * radius, center, up);


			const float near_clip = camera.znear;
			const float clip_range = camera.zfar - camera.znear;
			cascades_data[ctx.curr_frame_idx].distance[c] = (near_clip + distance_curr_cascade * clip_range) * -1.0f;
			cascades_data[ctx.curr_frame_idx].dir_light_view_proj[c] = cascade_proj * cascade_view;
		}

		cascades_data[ctx.curr_frame_idx].show_debug_view = show_debug_view;
		ssbo_cascades_data[ctx.curr_frame_idx].upload(ctx.device, &cascades_data[ctx.curr_frame_idx], 0, sizeof(CascadesData));
	}

	void show_ui()
	{
		if (ImGui::Begin("Shadow Renderer"))
		{
			ImGui::SeparatorText("Controls");
			ImGui::InputFloat("Lambda", &lambda);
			ImGui::InputFloat("[DEBUG] Radius Multiplier", &debug_radius_multiplier);

			ImGui::Checkbox("Show Debug View", &show_debug_view);

			for (int i = 0; i < k_num_cascades; i++)
			{
				ImGui::Text("Cascade %i: Distance = %f Radius = %f",i, cascades_data[ctx.curr_frame_idx].distance[i], debug_radius[i]);
				ImGui::ImageButton(shadow_cascades_view_ui_id[ctx.curr_frame_idx][i], { 256, 256 });
			}
		}
		ImGui::End();
	}

	static inline bool is_initialized = false;
	struct CascadesData
	{
		glm::mat4 dir_light_view_proj[k_num_cascades];
		float distance[k_num_cascades];					// Distance from near-plane for each cascade
		unsigned num_cascades;
		bool show_debug_view;
	};

	std::array<CascadesData, NUM_FRAMES> cascades_data;

	float lambda = 1.0f; // Interpolation factor controlling the mix between uniform and logarithmic distribution for cascade splits.

	static inline std::array<Texture2D, NUM_FRAMES> shadow_cascades_depth;	// Each element is a Texture2DArray storing k_num_cascades cascades
	std::array<vk::renderpass_dynamic, NUM_FRAMES> renderpass;
	std::array<vk::buffer, NUM_FRAMES> ssbo_cascades_data;

	VkDescriptorPool descriptor_pool;
	static inline vk::descriptor_set_layout descriptor_set_layout;
	static inline std::array<vk::descriptor_set, NUM_FRAMES> descriptor_set;

	VkImageView shadow_cascades_view[NUM_FRAMES][k_num_cascades];	// For each frame, each element is a view to a layer in the shadow map texture array
	ImTextureID shadow_cascades_view_ui_id[NUM_FRAMES][k_num_cascades];

	DrawMetricsEntry draw_metrics;

	// To remove
	// Debug only
	float debug_radius[k_num_cascades] = { 0.0f };
	float debug_radius_multiplier = 1.0f;
	bool show_debug_view = false;
	std::array<float, k_num_cascades> z_splits;
};