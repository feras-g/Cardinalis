#pragma once

#include "DebugLineRenderer.hpp"
#include "core/rendering/Material.hpp"
#include "core/rendering/vulkan/VulkanUI.h"
#include "core/rendering/vulkan/Renderers/IBLPrefiltering.hpp"
#include "core/rendering/vulkan/Renderers/ShadowRenderer.hpp"

#include "core/rendering/lighting.h"


struct DeferredRenderer : public IRenderer
{
	static constexpr int render_size = 2048;
	static constexpr float inv_render_size = 1.0f / render_size;

	VkFormat base_color_format = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat normal_format = VK_FORMAT_R16G16_SFLOAT;	// Only store X and Y components for View Space normals
	VkFormat metalness_roughness_format = VK_FORMAT_R8G8_UNORM;
	VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
	VkFormat light_accumulation_format = VK_FORMAT_R8G8B8A8_SRGB;
	
	const int light_volume_type_directional = 1;
	const int light_volume_type_point = 2;
	const int light_volume_type_spot = 3;

	static inline struct GBuffer
	{
		Texture2D base_color_attachment;
		Texture2D normal_attachment;
		Texture2D metalness_roughness_attachment;
		Texture2D depth_attachment;
		Texture2D light_accumulation_attachment;
		Texture2D final_lighting;
	} gbuffer[NUM_FRAMES];

	void init()
	{
		name = "Deferred Renderer";
		draw_metrics = DrawMetricsManager::add_entry(name.c_str());
		init_gbuffer();
		create_renderpass();
		create_pipeline();
		init_ui();
	}

	void init_gbuffer()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			gbuffer[i].base_color_attachment.init(base_color_format, render_size, render_size, 1, false, "[Deferred Renderer] Base Color Attachment");
			gbuffer[i].normal_attachment.init(normal_format, render_size, render_size, 1, false, "[Deferred Renderer] Packed XY Normals Attachment");
			gbuffer[i].metalness_roughness_attachment.init(metalness_roughness_format, render_size, render_size, 1, false, "[Deferred Renderer] Metalness Roughness Attachment");
			gbuffer[i].depth_attachment.init(depth_format, render_size, render_size, 1, false, "[Deferred Renderer] Depth Attachment");
			gbuffer[i].light_accumulation_attachment.init(light_accumulation_format, render_size, render_size, 1, false, "[Deferred Renderer] Lighting Accumulation");

			gbuffer[i].base_color_attachment.create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].normal_attachment.create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].metalness_roughness_attachment.create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].depth_attachment.create(ctx.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].light_accumulation_attachment.create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

			/* Create final attachment compositing all geometry information */
			gbuffer[i].final_lighting.init(VulkanRendererCommon::get_instance().swapchain_color_format, render_size, render_size, 1, 0, "[Deferred Renderer] Composite Color Attachment");
			gbuffer[i].final_lighting.create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
	}

	void create_pipeline()
	{
		create_pipeline_geometry_pass();
		create_pipeline_lighting_pass();
	}

	void create_renderpass() override
	{
		create_geometry_renderpass();
		create_lighting_renderpass();
	}

	void create_geometry_renderpass()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			renderpass_geometry[i].reset();
			renderpass_geometry[i].add_color_attachment(gbuffer[i].base_color_attachment.view, VK_ATTACHMENT_LOAD_OP_CLEAR);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].normal_attachment.view, VK_ATTACHMENT_LOAD_OP_CLEAR);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].metalness_roughness_attachment.view, VK_ATTACHMENT_LOAD_OP_CLEAR);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].light_accumulation_attachment.view, VK_ATTACHMENT_LOAD_OP_CLEAR);	// Render emissive materials to it
			renderpass_geometry[i].add_depth_attachment(gbuffer[i].depth_attachment.view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		}
	}

	void create_pipeline_geometry_pass()
	{
		VkFormat attachment_formats[]
		{
			base_color_format,
			normal_format,
			metalness_roughness_format,
			light_accumulation_format,
		};

		VkDescriptorSetLayout descriptor_set_layouts[] =
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			ObjectManager::get_instance().m_descriptor_set_bindless_textures.layout.vk_set_layout,
			ObjectManager::get_instance().mesh_descriptor_set_layout,
		};

		geometry_pass_pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
		geometry_pass_pipeline.layout.add_push_constant_range("Material", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(Material) });

		geometry_pass_pipeline.layout.create(descriptor_set_layouts);
		shader_geometry_pass.create("instanced_mesh_vert.vert.spv", "deferred_geometry_pass_frag.frag.spv");

		Pipeline::Flags flags = Pipeline::Flags::ENABLE_DEPTH_STATE;

		geometry_pass_pipeline.create_graphics(shader_geometry_pass, attachment_formats, depth_format, flags, geometry_pass_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void create_lighting_renderpass()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			renderpass_lighting[i].reset();
			renderpass_lighting[i].add_color_attachment(gbuffer[i].light_accumulation_attachment.view, VK_ATTACHMENT_LOAD_OP_LOAD); // Load because this might have been written to in the geometry pass
		}
	}

	void create_pipeline_lighting_pass();

	/* Render pass writing geometry information to G-Buffers */
	void render_geometry_pass(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Geometry Pass");

		set_viewport_scissor(cmd_buffer, render_size, render_size, true);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline);

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[ctx.curr_frame_idx].vk_set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 1, 1, &ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set, 0, nullptr);

		/* TODO : batch transitions ? */
		gbuffer[ctx.curr_frame_idx].base_color_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[ctx.curr_frame_idx].normal_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[ctx.curr_frame_idx].metalness_roughness_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[ctx.curr_frame_idx].depth_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
		gbuffer[ctx.curr_frame_idx].light_accumulation_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);


		renderpass_geometry[ctx.curr_frame_idx].begin(cmd_buffer, { render_size, render_size });
		ObjectManager& object_manager = ObjectManager::get_instance();

		for (size_t mesh_idx : mesh_list)
		{
			/* Mesh descriptor set */
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 2, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

			const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

			uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();

			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = mesh.geometry_data.primitives[prim_idx];

				geometry_pass_pipeline.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
				geometry_pass_pipeline.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

				vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
			}
		}

		renderpass_geometry[ctx.curr_frame_idx].end(cmd_buffer);


		/* TODO : batch transitions ? */
		gbuffer[ctx.curr_frame_idx].base_color_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[ctx.curr_frame_idx].normal_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[ctx.curr_frame_idx].metalness_roughness_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[ctx.curr_frame_idx].depth_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	}

	/* Renders a volume for each type of light, corresponding to its influence */
	void render_light_volumes()
	{

	}

	/* Compositing render pass using G-Buffers to compute lighting and render to fullscreen quad */
	void render_lighting_pass(VkCommandBuffer cmd_buffer)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline);

		glm::vec2 render_size = { gbuffer[ctx.curr_frame_idx].light_accumulation_attachment.info.width, gbuffer[ctx.curr_frame_idx].light_accumulation_attachment.info.height };

		// Invert when drawing to swapchain
		set_viewport_scissor(cmd_buffer, (uint32_t)render_size.x, (uint32_t)render_size.y, true);

		VkDescriptorSet bound_descriptor_sets[]
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set[ctx.curr_frame_idx].vk_set,
			sampled_images_descriptor_set[ctx.curr_frame_idx].vk_set,
			ObjectManager::get_instance().m_descriptor_sets[light_manager::directional_light_volume_mesh_id],
			light_manager::descriptor_set[ctx.curr_frame_idx].vk_set,
			ShadowRenderer::descriptor_set[ctx.curr_frame_idx].vk_set,
		};

		// Draw light volumes
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline.layout, 0, 5, bound_descriptor_sets, 0, nullptr);

		gbuffer[ctx.curr_frame_idx].light_accumulation_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		renderpass_lighting[ctx.curr_frame_idx].begin(cmd_buffer, { render_size.x, render_size.y });

		ObjectManager& object_manager = ObjectManager::get_instance();
		const glm::mat4 identity = glm::identity<glm::mat4>();

		// Draw directional light volume
		{
			light_volume_additional_data.light_type = light_volume_type_directional;

			const VulkanMesh& mesh_fs_quad = object_manager.m_meshes[light_manager::directional_light_volume_mesh_id];

			lighting_pass_pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass View Proj", &identity);
			lighting_pass_pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass Additional Data", &light_volume_additional_data);
			
			vkCmdDraw(cmd_buffer, (uint32_t)mesh_fs_quad.m_num_vertices, 1, 0, 0);
		}

		// Draw point light volumes
		{
			light_volume_additional_data.light_type = light_volume_type_point;
			glm::mat4 view_proj = VulkanRendererCommon::get_instance().m_framedata[ctx.curr_frame_idx].view_proj;

			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline.layout, 2, 1, &ObjectManager::get_instance().m_descriptor_sets[light_manager::point_light_volume_mesh_id].vk_set, 0, nullptr);
			const VulkanMesh& mesh_sphere = object_manager.m_meshes[light_manager::point_light_volume_mesh_id];
			uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[light_manager::point_light_volume_mesh_id].size();
			lighting_pass_pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass View Proj", &view_proj);
			lighting_pass_pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass Additional Data", &light_volume_additional_data);
			vkCmdDraw(cmd_buffer, (uint32_t)mesh_sphere.m_num_vertices, instance_count, 0, 0);
		}
		renderpass_lighting[ctx.curr_frame_idx].end(cmd_buffer);

		gbuffer[ctx.curr_frame_idx].light_accumulation_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	}


	void render(VkCommandBuffer cmd_buffer)
	{

	}

	void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Pass");

		render_geometry_pass(cmd_buffer, mesh_list);
		render_lighting_pass(cmd_buffer);
	}

	void update_shader_toggles()
	{
		
	}

	struct UITextureIDs
	{
		ImTextureID base_color;
		ImTextureID normal;
		ImTextureID metalness_roughness;
		ImTextureID depth;
		ImTextureID light_accumulation;
		ImTextureID final_lighting;
	} ui_texture_ids[NUM_FRAMES];

	void init_ui()
	{
		VkSampler& sampler_clamp_linear = VulkanRendererCommon::get_instance().s_SamplerClampLinear;

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			ui_texture_ids[i].base_color = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].base_color_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].normal = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].normal_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].metalness_roughness = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].metalness_roughness_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].depth = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].depth_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].light_accumulation = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].light_accumulation_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].final_lighting = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer[i].final_lighting.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}
	}

	void show_ui() override
	{

	}

	void show_ui(camera camera)
	{
		const ImVec2 main_img_size = { render_size, render_size };
		const ImVec2 thumb_img_size = { 256, 256 };

		static ImTextureID curr_main_image = ui_texture_ids[ctx.curr_frame_idx].light_accumulation;

		if (ImGui::Begin("GBuffer View"))
		{
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx].base_color, thumb_img_size);
			ImGui::SameLine();
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx].normal, thumb_img_size);
			ImGui::SameLine();
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx].metalness_roughness, thumb_img_size);
			ImGui::SameLine();
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx].light_accumulation, { thumb_img_size.x, thumb_img_size.y });
			ImGui::SameLine();
			ImGui::Image(ui_texture_ids[ctx.curr_frame_idx].depth, { thumb_img_size.x, thumb_img_size.y });

		}
		ImGui::End();

		if (ImGui::Begin("Deferred Renderer Toolbar"))
		{
			static bool reload_success = true;
			if (ImGui::Button("Reload Shaders"))
			{
				reload_success = reload_pipeline();
			}

			if (reload_success)
			{
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Pipeline reload success");
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Pipeline reload fail.");
			}
		}

		cubemap_renderer.show_ui();

		ImGui::End();
	}

	bool reload_pipeline() override
	{
		vkDeviceWaitIdle(ctx.device);

		if (shader_geometry_pass.compile())
		{
			geometry_pass_pipeline.reload_pipeline();
		}
		else
		{
			return false;
		}

		if (shader_lighting_pass.compile())
		{
			lighting_pass_pipeline.reload_pipeline();
		}
		else
		{
			return false;
		}


		if (cubemap_renderer.write_cubemap_shader.compile())
		{
			cubemap_renderer.pipeline.reload_pipeline();
			cubemap_renderer.render();
		}
		else
		{
			return false;
		}

		return true;
	}


	// Geometry pass
	Pipeline geometry_pass_pipeline;
	VertexFragmentShader shader_geometry_pass;
	vk::renderpass_dynamic renderpass_geometry[NUM_FRAMES];

	// Lighting pass
	struct light_volume_pass_data
	{
		float inv_screen_size;
		int light_type;
	} light_volume_additional_data;

	Pipeline lighting_pass_pipeline;
	VertexFragmentShader shader_lighting_pass;
	vk::renderpass_dynamic renderpass_lighting[NUM_FRAMES];
	
	vk::descriptor_set sampled_images_descriptor_set[NUM_FRAMES];
	vk::descriptor_set_layout sampled_images_descriptor_set_layout;

	vk::renderpass_dynamic renderpass[NUM_FRAMES];
	bool render_ok = false;

	VkDescriptorPool descriptor_pool;
	vk::descriptor_set descriptor_set;

	/* Contains different state toggles used inside fragment shader */
	struct ShaderToggles
	{
		
	} shader_params;

	DrawMetricsEntry draw_metrics;
};