#pragma once

#include "IRenderer.h"
#include "DeferredRenderer.hpp"

// WIP 
#include "PostFXRenderer.hpp"
// WIP

struct VolumetricLightRenderer : IRenderer
{
	static constexpr VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

	virtual void init()
	{
		create_pipeline();
		create_renderpass();

		// WIP
		gaussian_blur_renderer.init();
		gaussian_blur_renderer.set_source(volumetric_lighting_attachment[0]);
		// WIP

		is_initialized = true;
	}

	virtual void create_pipeline()
	{
		volumetric_descriptor_set_layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, "Light data");
		volumetric_descriptor_set_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Deferred Depth Buffer");
		volumetric_descriptor_set_layout.create("Volumetric Lighting Common Descriptor Set Layout");

		create_pipeline_volumetric_sunlight();
		create_pipeline_volumetric_point_lights();
	}

	void create_pipeline_volumetric_sunlight()
	{
		VkDescriptorSetLayout descriptor_set_layouts[] =
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout, 
			volumetric_descriptor_set_layout,
			ObjectManager::get_instance().mesh_descriptor_set_layout,
			ShadowRenderer::descriptor_set_layout, 
		};

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			volumetric_sunlight_descriptor_set[i].assign_layout(volumetric_descriptor_set_layout);
			volumetric_sunlight_descriptor_set[i].create("");
			volumetric_sunlight_descriptor_set[i].write_descriptor_storage_buffer(0, light_manager::ssbo[i], 0, sizeof(directional_light));
			volumetric_sunlight_descriptor_set[i].write_descriptor_combined_image_sampler(1, DeferredRenderer::gbuffer[i].depth_attachment.view, VulkanRendererCommon::get_instance().smp_clamp_nearest);
		}

		volumetric_sunlight_pipeline.layout.add_push_constant_range("Sunlight Push Constants Vertex", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(ps_vertex) });
		volumetric_sunlight_pipeline.layout.add_push_constant_range("Sunlight Push Constants Fragment", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(ps_vertex), .size = sizeof(ps_fragment)});

		volumetric_sunlight_pipeline.layout.create(descriptor_set_layouts);
		volumetric_sunlight_shader.create("Volumetric Sunlight", "render_light_volume_vert.vert.spv", "volumetric_sunlight_frag.frag.spv");

		VkFormat color_formats[]{ color_format };
		volumetric_sunlight_pipeline.create_graphics(volumetric_sunlight_shader, color_formats, {}, Pipeline::Flags::ENABLE_ALPHA_BLENDING, volumetric_sunlight_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_POLYGON_MODE_FILL);
	}

	void create_pipeline_volumetric_point_lights()
	{
		VkDescriptorSetLayout descriptor_set_layouts[] =
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			volumetric_descriptor_set_layout,
			ObjectManager::get_instance().mesh_descriptor_set_layout,
		};

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			volumetric_point_light_descriptor_set[i].assign_layout(volumetric_descriptor_set_layout);
			volumetric_point_light_descriptor_set[i].create("");
			volumetric_point_light_descriptor_set[i].write_descriptor_storage_buffer(0, light_manager::ssbo[i], sizeof(directional_light), VK_WHOLE_SIZE);
			volumetric_point_light_descriptor_set[i].write_descriptor_combined_image_sampler(1, DeferredRenderer::gbuffer[i].depth_attachment.view, VulkanRendererCommon::get_instance().smp_clamp_nearest);
		}

		volumetric_point_light_pipeline.layout.add_push_constant_range("Light Volume View Proj", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
		volumetric_point_light_pipeline.layout.add_push_constant_range("Inv Screen Size", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(float) });

		volumetric_point_light_pipeline.layout.create(descriptor_set_layouts);
		volumetric_point_light_shader.create("Volumetric Pointlight", "render_light_volume_vert.vert.spv", "volumetric_point_light_frag.frag.spv");

		VkFormat color_formats[]{ color_format };
		volumetric_point_light_pipeline.create_graphics(volumetric_point_light_shader, color_formats, {}, Pipeline::Flags::ENABLE_ALPHA_BLENDING, volumetric_point_light_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_POLYGON_MODE_FILL);
	}

	virtual void create_renderpass()
	{
		// We render the volumetric light to a fraction of the full resolution
		ps_fragment.downsample_factor = 2; 
		render_size = glm::vec2(DeferredRenderer::render_size / ps_fragment.downsample_factor);

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			volumetric_sunlight_ubo[i].init(vk::buffer::type::UNIFORM, sizeof(directional_light), "");
			volumetric_lighting_attachment[i].init(color_format, render_size, 1, 0, "Volumetric Sunlight Attachment");
			volumetric_lighting_attachment[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
			ui_texture_ids[i] = ImGui_ImplVulkan_AddTexture(VulkanRendererCommon::get_instance().smp_clamp_nearest, volumetric_lighting_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			renderpass[i].add_color_attachment(volumetric_lighting_attachment[i].view);
		}
	}

	void render_volumetric_sunlight(VkCommandBuffer cmd_buffer, uint32_t frame_index)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Volumetric Sunlight Pass");

		const VulkanMesh& mesh_fs_quad = ObjectManager::get_instance().m_meshes[light_manager::directional_light_volume_mesh_id];

		std::array<VkDescriptorSet, 4> bound_descriptor_sets
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set[frame_index].vk_set,
			volumetric_sunlight_descriptor_set[frame_index].vk_set,
			ObjectManager::get_instance().m_descriptor_sets[light_manager::directional_light_volume_mesh_id],
			ShadowRenderer::descriptor_set[frame_index].vk_set
		};

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volumetric_sunlight_pipeline.layout, 0, (uint32_t)bound_descriptor_sets.size(), bound_descriptor_sets.data(), 0, nullptr);

		ps_vertex.light_volume_view_proj = mat_identity;
		volumetric_sunlight_pipeline.cmd_push_constants(cmd_buffer, "Sunlight Push Constants Vertex", &ps_vertex);

		ps_fragment.inv_deferred_render_size = DeferredRenderer::inv_render_size;
		volumetric_sunlight_pipeline.cmd_push_constants(cmd_buffer, "Sunlight Push Constants Fragment", &ps_fragment);
		vkCmdDraw(cmd_buffer, (uint32_t)mesh_fs_quad.m_num_vertices, 1, 0, 0);
	}

	void render_volumetric_point_lights(VkCommandBuffer cmd_buffer, uint32_t frame_index)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Volumetric Point Lights Pass");

		const VulkanMesh& mesh_sphere = ObjectManager::get_instance().m_meshes[light_manager::point_light_volume_mesh_id];

		std::array<VkDescriptorSet, 3> bound_descriptor_sets
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set[frame_index].vk_set,
			volumetric_point_light_descriptor_set[frame_index].vk_set,
			ObjectManager::get_instance().m_descriptor_sets[light_manager::point_light_volume_mesh_id],
		};

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volumetric_point_light_pipeline.layout, 0, (uint32_t)bound_descriptor_sets.size(), bound_descriptor_sets.data(), 0, nullptr);
		const glm::mat4 view_proj = VulkanRendererCommon::get_instance().m_framedata[ctx.curr_frame_idx].view_proj;
		volumetric_point_light_pipeline.cmd_push_constants(cmd_buffer, "Light Volume View Proj", &view_proj);
		volumetric_point_light_pipeline.cmd_push_constants(cmd_buffer, "Inv Screen Size", &DeferredRenderer::inv_render_size);

		uint32_t instance_count = (uint32_t)ObjectManager::get_instance().m_mesh_instance_data[light_manager::point_light_volume_mesh_id].size();
		vkCmdDraw(cmd_buffer, (uint32_t)mesh_sphere.m_num_vertices, instance_count, 0, 0);
	}

	virtual void render(VkCommandBuffer cmd_buffer)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Volumetric Lighting Pass");
		volumetric_sunlight_pipeline.bind(cmd_buffer);
		uint32_t frame_index = ctx.curr_frame_idx;

		set_viewport_scissor(cmd_buffer, (uint32_t)render_size.x, (uint32_t)render_size.y, true);
		volumetric_lighting_attachment[frame_index].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		renderpass[frame_index].begin(cmd_buffer, render_size);

		render_volumetric_sunlight(cmd_buffer, frame_index);
		//render_volumetric_point_lights(cmd_buffer, frame_index);

		renderpass[frame_index].end(cmd_buffer);

		// WIP 
		volumetric_lighting_attachment[frame_index].transition(cmd_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT);
		apply_blur(cmd_buffer);
		// WIP

		volumetric_lighting_attachment[frame_index].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	}

	/* Gaussian Blur */
	void apply_blur(VkCommandBuffer cmd_buffer)
	{
		gaussian_blur_renderer.execute(cmd_buffer, "Volumetric Fog Blur Pass");
	}

	virtual void show_ui()
	{
		if (ImGui::Begin("Volumetric Light Renderer"))
		{
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx], { render_size.x, render_size.y });

			ImGui::SeparatorText("Volumetric Sunlight Parameters");
			ImGui::SliderInt("Num Raymarch Steps", &ps_fragment.scattering_params.num_raymarch_steps, 0, 200);
			ImGui::SliderFloat("Mie Scattering 'g'", &ps_fragment.scattering_params.g_mie, 0.0f, 1.0f);
			ImGui::SliderFloat("Scattering Amount", &ps_fragment.scattering_params.amount, 0.0f, 100.0f);
		}
		ImGui::End();

		// WIP 
		gaussian_blur_renderer.show_ui();
		// WIP
	}

	virtual bool reload_pipeline() { return true; }

	struct ScatteringParameters
	{
		float g_mie  = 0.5f;				/* Float value between 0 and 1 controlling how much light will scatter in the forward direction. (Henyey-Greenstein phase function) */
		float amount = 1.0f;				/* Float value controlling how much volumetric light is visible. */
		int num_raymarch_steps = 25;
	};

	struct PushConstantsVertexShader
	{
		glm::mat4 light_volume_view_proj;
	} ps_vertex;

	struct Sunlight_PushConstantsFragmentShader
	{
		float downsample_factor;
		float inv_deferred_render_size;
		ScatteringParameters scattering_params;
	} ps_fragment;

	glm::vec2 render_size;

	vk::descriptor_set_layout volumetric_descriptor_set_layout;

	static inline bool is_initialized = false;

	static inline std::array<Texture2D, NUM_FRAMES> volumetric_lighting_attachment;
	VertexFragmentShader volumetric_sunlight_shader;
	Pipeline volumetric_sunlight_pipeline;
	std::array<vk::descriptor_set, NUM_FRAMES> volumetric_sunlight_descriptor_set;
	std::array<vk::buffer, NUM_FRAMES> volumetric_sunlight_ubo;
	std::array<ImTextureID, NUM_FRAMES> ui_texture_ids;
	std::array<vk::renderpass_dynamic, NUM_FRAMES> renderpass;
	VertexFragmentShader shader_volumetric_omnilight;

	static constexpr glm::mat4 mat_identity = glm::identity<glm::mat4>();
	VertexFragmentShader volumetric_point_light_shader;
	Pipeline volumetric_point_light_pipeline;
	std::array<vk::descriptor_set, NUM_FRAMES> volumetric_point_light_descriptor_set;



	// WIP
	static inline TwoPassGaussianBlur gaussian_blur_renderer;
	// WIP
};