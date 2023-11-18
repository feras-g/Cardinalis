#pragma once

#include "DebugLineRenderer.hpp"
#include "core/rendering/Material.hpp"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/rendering/vulkan/VulkanUI.h"


struct DeferredRenderer : public IRenderer
{
	static constexpr int render_size = 2048;

	VkFormat base_color_format = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat normal_format = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormat metalness_roughness_format = VK_FORMAT_R8G8_UNORM;
	VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

	struct GBuffer
	{
		Texture2D base_color_attachment;
		Texture2D normal_attachment;
		Texture2D metalness_roughness_attachment;
		Texture2D depth_attachment;
	} gbuffer[NUM_FRAMES];

	void init()
	{
		name = "Deferred Renderer";
		init_gbuffer();
		create_renderpass();
		create_pipeline();
		init_ui();
		deferred_renderer_stats = IRenderer::draw_stats.add_entry(name.c_str());
	}

	void init_gbuffer()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			gbuffer[i].base_color_attachment.init(base_color_format, render_size, render_size, 1, false, "[Deferred Renderer] Base Color Attachment");
			gbuffer[i].normal_attachment.init(normal_format, render_size, render_size, 1, false, "[Deferred Renderer] Packed XY Normals Attachment");
			gbuffer[i].metalness_roughness_attachment.init(metalness_roughness_format, render_size, render_size, 1, false, "[Deferred Renderer] Metalness Roughness Attachment");
			gbuffer[i].depth_attachment.init(depth_format, render_size, render_size, 1, false, "[Deferred Renderer] Depth Attachment");

			gbuffer[i].base_color_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].normal_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].metalness_roughness_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].depth_attachment.create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

			/* Create final attachment compositing all geometry information */
			compositing_attachments[i].init(VulkanRendererCommon::get_instance().swapchain_color_format, render_size, render_size, 1, 0, "[Deferred Renderer] Composite Color Attachment");
			compositing_attachments[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
	}

	void create_pipeline()
	{
		create_pipeline_gbuffer_pass();
		create_pipeline_lighting_pass();
	}

	void create_renderpass()
	{
		create_geometry_renderpass();
		create_lighting_renderpass();
	}

	void create_geometry_renderpass()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			renderpass_geometry[i].reset();
			renderpass_geometry[i].add_color_attachment(gbuffer[i].base_color_attachment.view);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].normal_attachment.view);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].metalness_roughness_attachment.view);
			renderpass_geometry[i].add_depth_attachment(gbuffer[i].depth_attachment.view);
		}
	}

	void create_lighting_renderpass()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			renderpass_lighting[i].reset();
			renderpass_lighting[i].add_color_attachment(VulkanImGuiRenderer::scene_viewport_attachments[i].view);
		}
	}

	void create_pipeline_gbuffer_pass()
	{
		VkFormat attachment_formats[]
		{
			base_color_format,
			normal_format,
			metalness_roughness_format,
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
		shader_geometry_pass.create("instanced_mesh_vert.vert.spv", "deferred_gbuffer_pass_frag.frag.spv");
		geometry_pass_pipeline.create_graphics(shader_geometry_pass, attachment_formats, depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, geometry_pass_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void create_pipeline_lighting_pass()
	{
		VkFormat attachment_formats[]
		{
			context.swapchain->info.color_format
		};

		std::array<VkDescriptorPoolSize, 1> pool_sizes
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

		VkSampler& sample_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		
		gbuffer_descriptor_layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sample_clamp_nearest, "GBuffer Base Color");
		gbuffer_descriptor_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sample_clamp_nearest, "GBuffer Normal");
		gbuffer_descriptor_layout.add_combined_image_sampler_binding(2, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sample_clamp_nearest, "GBuffer Metalness Roughness");
		gbuffer_descriptor_layout.add_combined_image_sampler_binding(3, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sample_clamp_nearest, "GBuffer Depth");
		gbuffer_descriptor_layout.create("GBuffer Descriptor Layout");

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			gbuffer_descriptor_set[i].assign_layout(gbuffer_descriptor_layout);
			gbuffer_descriptor_set[i].create(descriptor_pool, "GBuffer Descriptor Set");
			gbuffer_descriptor_set[i].write_descriptor_combined_image_sampler(0, gbuffer[i].base_color_attachment.view, sample_clamp_nearest);
			gbuffer_descriptor_set[i].write_descriptor_combined_image_sampler(1, gbuffer[i].normal_attachment.view, sample_clamp_nearest);
			gbuffer_descriptor_set[i].write_descriptor_combined_image_sampler(2, gbuffer[i].metalness_roughness_attachment.view, sample_clamp_nearest);
			gbuffer_descriptor_set[i].write_descriptor_combined_image_sampler(3, gbuffer[i].depth_attachment.view, sample_clamp_nearest);
		}

		VkDescriptorSetLayout descriptor_set_layouts[] =
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			gbuffer_descriptor_layout,
		};

		lighting_pass_pipeline.layout.create(descriptor_set_layouts);
		shader_lighting_pass.create("fullscreen_quad_vert.vert.spv", "deferred_lighting_pass_frag.frag.spv");
		lighting_pass_pipeline.create_graphics(shader_lighting_pass, attachment_formats, depth_format, Pipeline::Flags::NONE, lighting_pass_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	/* Render pass writing geometry information to G-Buffers */
	void render_geometry_pass(VkCommandBuffer cmd_buffer, const ObjectManager& object_manager)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Geometry Pass");

		set_viewport_scissor(cmd_buffer, render_size, render_size, true);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline);

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx].vk_set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 1, 1, &ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set, 0, nullptr);
		
		/* TODO : batch transitions ? */
		gbuffer[context.curr_frame_idx].base_color_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[context.curr_frame_idx].normal_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[context.curr_frame_idx].metalness_roughness_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		gbuffer[context.curr_frame_idx].depth_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

		renderpass_geometry[context.curr_frame_idx].begin(cmd_buffer, { render_size, render_size });
		for (size_t mesh_idx = 0; mesh_idx < object_manager.m_meshes.size(); mesh_idx++)
		{
			/* Mesh descriptor set */
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass_pipeline.layout, 2, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

			const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

			uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();
			deferred_renderer_stats.increment_instance_count(instance_count);

			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = mesh.geometry_data.primitives[prim_idx];
				deferred_renderer_stats.increment_vertex_count(p.vertex_count * instance_count);

				geometry_pass_pipeline.layout.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
				geometry_pass_pipeline.layout.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

				vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
				deferred_renderer_stats.increment_drawcall_count(1);
			}
		}
		renderpass_geometry[context.curr_frame_idx].end(cmd_buffer);


		/* TODO : batch transitions ? */
		gbuffer[context.curr_frame_idx].base_color_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[context.curr_frame_idx].normal_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[context.curr_frame_idx].metalness_roughness_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		gbuffer[context.curr_frame_idx].depth_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	}

	/* Compositing render pass using G-Buffers to compute lighting and render to fullscreen quad */
	void render_lighting_pass(VkCommandBuffer cmd_buffer)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline);

		set_viewport_scissor(cmd_buffer, VulkanImGuiRenderer::scene_viewport_size.x, VulkanImGuiRenderer::scene_viewport_size.y, false);

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx].vk_set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pass_pipeline.layout, 1, 1, &gbuffer_descriptor_set[context.curr_frame_idx].vk_set, 0, nullptr);

		compositing_attachments[context.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		renderpass_lighting[context.curr_frame_idx].begin(cmd_buffer, { VulkanImGuiRenderer::scene_viewport_size.x, VulkanImGuiRenderer::scene_viewport_size.y });
		vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
		renderpass_lighting[context.curr_frame_idx].end(cmd_buffer);
		compositing_attachments[context.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	}

	void render(VkCommandBuffer cmd_buffer)
	{

	}

	void render(VkCommandBuffer cmd_buffer, const ObjectManager& object_manager)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Pass");

		draw_stats.reset();

		render_geometry_pass(cmd_buffer, object_manager);
		render_lighting_pass(cmd_buffer);

		render_ok = true;
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
		ImTextureID composite;
	} ui_texture_ids[NUM_FRAMES];

	void init_ui()
	{
		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;

		for (int i = 0; i < NUM_FRAMES; i++)
		{
			ui_texture_ids[i].base_color = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, gbuffer[i].base_color_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].normal = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, gbuffer[i].normal_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].metalness_roughness = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, gbuffer[i].metalness_roughness_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].depth = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, gbuffer[i].depth_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			ui_texture_ids[i].composite = VulkanImGuiRenderer::scene_viewport_attachments_ids[i];
		}
	}

	void show_ui() override
	{
		if (render_ok)
		{
			const ImVec2 main_img_size  = { render_size, render_size };
			const ImVec2 thumb_img_size = { ((float)VulkanGUI::scene_viewport_window_size.x / 4 ), (float)VulkanGUI::scene_viewport_window_size.y / 4};

			static ImTextureID curr_main_image = ui_texture_ids[context.curr_frame_idx].composite;

			if (ImGui::Begin("GBuffer View"))
			{
				if (ImGui::ImageButton(ui_texture_ids[context.curr_frame_idx].base_color, thumb_img_size))
				{
					curr_main_image = ui_texture_ids[context.curr_frame_idx].base_color;
				}
				ImGui::SameLine();
				
				if (ImGui::ImageButton(ui_texture_ids[context.curr_frame_idx].normal, thumb_img_size))
				{
					curr_main_image = ui_texture_ids[context.curr_frame_idx].normal;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(ui_texture_ids[context.curr_frame_idx].metalness_roughness, thumb_img_size))
				{
					curr_main_image = ui_texture_ids[context.curr_frame_idx].metalness_roughness;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(ui_texture_ids[context.curr_frame_idx].depth, { thumb_img_size.x - 55, thumb_img_size.y }))
				{
					curr_main_image = ui_texture_ids[context.curr_frame_idx].depth;
				}
			}
			ImGui::End();

			if (ImGui::Begin("Deferred Renderer Toolbar"))
			{
				if (ImGui::Button("Reload Shaders"))
				{
					if (reload_pipeline())
					{
						ImGui::TextColored(ImVec4(0, 1, 0, 1), "Pipeline reload success");
					}
					else
					{
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "Pipeline reload fail.");
					}
				}
			}
			ImGui::End();
		}
	}

	bool reload_pipeline() override
	{
		vkDeviceWaitIdle(context.device);

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

		return true;
	}

	VertexFragmentShader shader_geometry_pass;
	VertexFragmentShader shader_lighting_pass;
	bool render_ok = false;
	VulkanRenderPassDynamic renderpass_geometry[NUM_FRAMES];
	VulkanRenderPassDynamic renderpass_lighting[NUM_FRAMES];
	Texture2D compositing_attachments[NUM_FRAMES];
	
	Pipeline geometry_pass_pipeline;
	Pipeline lighting_pass_pipeline;
	DescriptorSet gbuffer_descriptor_set[NUM_FRAMES];
	DescriptorSetLayout gbuffer_descriptor_layout;

	VulkanRenderPassDynamic renderpass[NUM_FRAMES];

	VkDescriptorPool descriptor_pool;
	DescriptorSet descriptor_set;
	/* Contains different state toggles used inside fragment shader */
	struct ShaderToggles
	{
		
	} shader_params;

	DrawStatsEntry deferred_renderer_stats;
};