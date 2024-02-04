#pragma once

#include "DeferredRenderer.hpp"

#include "core/rendering/vulkan/Renderers/VolumetricLightRenderer.hpp"

int DeferredRenderer::render_size = 2048;
float DeferredRenderer::inv_render_size = 1.0f / render_size;

VkFormat DeferredRenderer::base_color_format = VK_FORMAT_R8G8B8A8_SRGB;
VkFormat DeferredRenderer::normal_format = VK_FORMAT_R16G16_SFLOAT;	// Only store X and Y components for View Space normals
VkFormat DeferredRenderer::metalness_roughness_format = VK_FORMAT_R8G8_UNORM;
VkFormat DeferredRenderer::depth_format = VK_FORMAT_D32_SFLOAT;
VkFormat DeferredRenderer::light_accumulation_format = VK_FORMAT_R8G8B8A8_SRGB;

void DeferredRenderer::GBuffer::init()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		gbuffer.base_color_attachment[i].init(base_color_format, render_size, render_size, 1, false, "[Deferred Renderer] Base Color Attachment");
		gbuffer.normal_attachment[i].init(normal_format, render_size, render_size, 1, false, "[Deferred Renderer] Packed XY Normals Attachment");
		gbuffer.metalness_roughness_attachment[i].init(metalness_roughness_format, render_size, render_size, 1, false, "[Deferred Renderer] Metalness Roughness Attachment");
		gbuffer.depth_attachment[i].init(depth_format, render_size, render_size, 1, false, "[Deferred Renderer] Depth Attachment");
		gbuffer.light_accumulation_attachment[i].init(light_accumulation_format, render_size, render_size, 1, false, "[Deferred Renderer] Lighting Accumulation");

		gbuffer.base_color_attachment[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		gbuffer.normal_attachment[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		gbuffer.metalness_roughness_attachment[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		gbuffer.depth_attachment[i].create(ctx.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		gbuffer.light_accumulation_attachment[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		/* Create final attachment compositing all geometry information */
		gbuffer.final_lighting[i].init(VulkanRendererCommon::get_instance().swapchain_color_format, render_size, render_size, 1, 0, "[Deferred Renderer] Composite Color Attachment");
		gbuffer.final_lighting[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	}
}

void DeferredRenderer::UITextureIDs::init()
{
	VkSampler& sampler_clamp_linear = VulkanRendererCommon::get_instance().smp_clamp_linear;

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		ui_texture_ids.base_color[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.base_color_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		ui_texture_ids.normal[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.normal_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		ui_texture_ids.metalness_roughness[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.metalness_roughness_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		ui_texture_ids.depth[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.depth_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		ui_texture_ids.light_accumulation[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.light_accumulation_attachment[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		ui_texture_ids.final_lighting[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_linear, gbuffer.final_lighting[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}
}
void DeferredRenderer::init()
{
	name = "Deferred Renderer";
	draw_metrics = DrawMetricsManager::add_entry(name.c_str());
	GBuffer::init();
	UITextureIDs::init();
	create_renderpass();
	create_pipeline();
}

void DeferredRenderer::GeometryPass::create_pipeline()
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

	pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
	pipeline.layout.add_push_constant_range("Material", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(Material) });

	pipeline.layout.create(descriptor_set_layouts);
	shader.create("Deferred Shading - Geometry Pass", "instanced_mesh_vert.vert.spv", "deferred_geometry_pass_frag.frag.spv");

	Pipeline::Flags flags = Pipeline::Flags::ENABLE_DEPTH_STATE;

	pipeline.create_graphics(shader, attachment_formats, depth_format, flags, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}
void DeferredRenderer::GeometryPass::create_renderpass()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		renderpass[i].reset();
		renderpass[i].add_color_attachment(gbuffer.base_color_attachment[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		renderpass[i].add_color_attachment(gbuffer.normal_attachment[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		renderpass[i].add_color_attachment(gbuffer.metalness_roughness_attachment[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		renderpass[i].add_color_attachment(gbuffer.light_accumulation_attachment[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);	// Render emissive materials to it
		renderpass[i].add_depth_attachment(gbuffer.depth_attachment[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
	}
}
void DeferredRenderer::LightingPass::create_pipeline()
{
	VkFormat attachment_formats[]
	{
		light_accumulation_format
	};

	VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().smp_clamp_nearest;
	VkSampler& sampler_repeat_linear = VulkanRendererCommon::get_instance().smp_repeat_linear;
	VkSampler& sampler_clamp_linear = VulkanRendererCommon::get_instance().smp_clamp_linear;
	VkSampler& sampler_repeat_nearest = VulkanRendererCommon::get_instance().smp_repeat_nearest;

	/*
		Create descriptor set bindings
	*/
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Base Color");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Normal");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(2, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Metalness Roughness");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(3, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Depth");

	/* Add images from image-based lighting */
	if (IBLRenderer::is_initialized)
	{
		// Only use a nearest sampler to sample these image, linear introduces artifacts probably due to averaging texels
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(4, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Pre-filtered Env Map Diffuse");
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(5, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Pre-filtered Env Map Specular");
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(6, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "BRDF Integration Map");
	}

	/* Add images from Volumetric Light renderer */
	if (VolumetricLightRenderer::is_initialized)
	{
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(7, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Volumetric Lighting");
	}

	sampled_images_descriptor_set_layout.create("GBuffer Descriptor Layout");

	/*
		Write to bindings
	*/
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		sampled_images_descriptor_set[i].assign_layout(sampled_images_descriptor_set_layout);
		sampled_images_descriptor_set[i].create("GBuffer Descriptor Set");
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(0, gbuffer.base_color_attachment[i].view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(1, gbuffer.normal_attachment[i].view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(2, gbuffer.metalness_roughness_attachment[i].view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(3, gbuffer.depth_attachment[i].view, sampler_clamp_nearest);

		if (IBLRenderer::is_initialized)
		{
			// Only use a nearest sampler to sample these image, linear introduces artifacts probably due to averaging texels
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(4, IBLRenderer::prefiltered_diffuse_env_map.view, sampler_repeat_nearest);
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(5, IBLRenderer::prefiltered_specular_env_map.view, sampler_repeat_nearest);
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(6, IBLRenderer::brdf_integration_map.view, sampler_repeat_nearest);
		}

		if (VolumetricLightRenderer::is_initialized)
		{
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(7, VolumetricLightRenderer::gaussian_blur_renderer.destination[i].view, sampler_repeat_linear);
		}
	}

	VkDescriptorSetLayout descriptor_set_layouts[] =
	{
		VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
		sampled_images_descriptor_set_layout,
		ObjectManager::get_instance().mesh_descriptor_set_layout,
		light_manager::descriptor_set_layout,
		ShadowRenderer::descriptor_set_layout,
	};


	light_volume_additional_data.inv_screen_size = 1.0f / render_size;

	pipeline.layout.add_push_constant_range("Light Volume Pass View Proj", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
	pipeline.layout.add_push_constant_range("Light Volume Pass Additional Data", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(light_volume_additional_data) });

	pipeline.layout.create(descriptor_set_layouts);
	shader.create("Deferred Shading - Lighting Pass", "render_light_volume_vert.vert.spv", "deferred_lighting_pass_frag.frag.spv");
	pipeline.create_graphics(shader, attachment_formats, {}, Pipeline::Flags::ENABLE_ALPHA_BLENDING, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_POLYGON_MODE_FILL);
}
void DeferredRenderer::LightingPass::create_renderpass()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		renderpass[i].reset();
		renderpass[i].add_color_attachment(gbuffer.light_accumulation_attachment[i].view, VK_ATTACHMENT_LOAD_OP_LOAD); // Load because this might have been written to in the geometry pass
	}
}

void DeferredRenderer::create_pipeline()
{
	geometry_pass.create_pipeline();
	lighting_pass.create_pipeline();
}
void DeferredRenderer::create_renderpass()
{
	geometry_pass.create_renderpass();
	lighting_pass.create_renderpass();
}
void DeferredRenderer::render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Shading");
	geometry_pass.render(cmd_buffer, mesh_list);
	lighting_pass.render(cmd_buffer);
}

void DeferredRenderer::show_ui(camera camera)
{
	const ImVec2 main_img_size = { (float)render_size, (float)render_size };
	const ImVec2 thumb_img_size = { 256, 256 };

	static ImTextureID curr_main_image = ui_texture_ids.light_accumulation[ctx.curr_frame_idx];

	if (ImGui::Begin("GBuffer View"))
	{
		ImGui::Image(ui_texture_ids.base_color[ctx.curr_frame_idx], thumb_img_size);
		ImGui::SameLine();
		ImGui::Image(ui_texture_ids.normal[ctx.curr_frame_idx], thumb_img_size);
		ImGui::SameLine();
		ImGui::Image(ui_texture_ids.metalness_roughness[ctx.curr_frame_idx], thumb_img_size);
		ImGui::SameLine();
		ImGui::Image(ui_texture_ids.light_accumulation[ctx.curr_frame_idx], { thumb_img_size.x, thumb_img_size.y });
		ImGui::SameLine();
		ImGui::Image(ui_texture_ids.depth[ctx.curr_frame_idx], { thumb_img_size.x, thumb_img_size.y });

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

void DeferredRenderer::show_ui()
{

}

bool DeferredRenderer::reload_pipeline()
{
	vkDeviceWaitIdle(ctx.device);

	if (geometry_pass.shader.compile())
	{
		geometry_pass.pipeline.reload_pipeline();
	}
	else
	{
		return false;
	}

	if (lighting_pass.shader.compile())
	{
		lighting_pass.pipeline.reload_pipeline();
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

void DeferredRenderer::GeometryPass::render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Geometry Pass");

	set_viewport_scissor(cmd_buffer, render_size, render_size, true);

	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[ctx.curr_frame_idx].vk_set, 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set, 0, nullptr);

	/* TODO : batch transitions ? */
	gbuffer.base_color_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	gbuffer.normal_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	gbuffer.metalness_roughness_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	gbuffer.depth_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	gbuffer.light_accumulation_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);


	renderpass[ctx.curr_frame_idx].begin(cmd_buffer, { render_size, render_size });
	ObjectManager& object_manager = ObjectManager::get_instance();

	for (size_t mesh_idx : mesh_list)
	{
		/* Mesh descriptor set */
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 2, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

		const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

		uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();

		for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
		{
			const Primitive& p = mesh.geometry_data.primitives[prim_idx];

			pipeline.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
			pipeline.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

			vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
		}
	}
	renderpass[ctx.curr_frame_idx].end(cmd_buffer);

	/* TODO : batch transitions ? */
	gbuffer.base_color_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	gbuffer.normal_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	gbuffer.metalness_roughness_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	gbuffer.depth_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
}

void DeferredRenderer::LightingPass::render(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	glm::vec2 render_size = { gbuffer.light_accumulation_attachment[0].info.width, gbuffer.light_accumulation_attachment[0].info.height};

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
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 5, bound_descriptor_sets, 0, nullptr);

	gbuffer.light_accumulation_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	renderpass[ctx.curr_frame_idx].begin(cmd_buffer, { render_size.x, render_size.y });

	ObjectManager& object_manager = ObjectManager::get_instance();
	const glm::mat4 identity = glm::identity<glm::mat4>();

	// Draw directional light volume
	{
		light_volume_additional_data.light_type = light_volume_type_directional;

		const VulkanMesh& mesh_fs_quad = object_manager.m_meshes[light_manager::directional_light_volume_mesh_id];

		pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass View Proj", &identity);
		pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass Additional Data", &light_volume_additional_data);

		vkCmdDraw(cmd_buffer, (uint32_t)mesh_fs_quad.m_num_vertices, 1, 0, 0);
	}

	// Draw point light volumes
	{
		light_volume_additional_data.light_type = light_volume_type_point;
		glm::mat4 view_proj = VulkanRendererCommon::get_instance().m_framedata[ctx.curr_frame_idx].view_proj;

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 2, 1, &ObjectManager::get_instance().m_descriptor_sets[light_manager::point_light_volume_mesh_id].vk_set, 0, nullptr);
		const VulkanMesh& mesh_sphere = object_manager.m_meshes[light_manager::point_light_volume_mesh_id];
		uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[light_manager::point_light_volume_mesh_id].size();
		pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass View Proj", &view_proj);
		pipeline.cmd_push_constants(cmd_buffer, "Light Volume Pass Additional Data", &light_volume_additional_data);
		vkCmdDraw(cmd_buffer, (uint32_t)mesh_sphere.m_num_vertices, instance_count, 0, 0);
	}
	renderpass[ctx.curr_frame_idx].end(cmd_buffer);

	gbuffer.light_accumulation_attachment[ctx.curr_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}

