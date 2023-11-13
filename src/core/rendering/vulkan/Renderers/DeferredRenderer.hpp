#pragma once

#include "DebugLineRenderer.hpp"
#include "core/rendering/Material.hpp"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/rendering/vulkan/VulkanUI.h"


struct DeferredRenderer : public IRenderer
{
	static constexpr render_size = 1024;

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
		deferred_renderer_stats = IRenderer::draw_stats.add_entry(name.c_str());
	}

	void init_gbuffer()
	{
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			gbuffer[i].base_color_attachment.init(VK_FORMAT_R8G8B8A8_SRGB, render_size, render_size, 1, false, "[Deferred Renderer] Base Color Attachment");
			gbuffer[i].normal_attachment.init(VK_FORMAT_R16G16_SNORM, render_size, render_size, 1, false, "[Deferred Renderer] Packed XY Normals Attachment");
			gbuffer[i].metalness_roughness_attachment.init(VK_FORMAT_R8G8_UNORM, render_size, render_size, 1, false, "[Deferred Renderer] Metalness Roughness Attachment");
			gbuffer[i].depth_attachment.init(VK_FORMAT_D32_SFLOAT, render_size, render_size, 1, false, "[Deferred Renderer] Depth Attachment");

			gbuffer[i].base_color_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].normal_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].metalness_roughness_attachment.create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			gbuffer[i].depth_attachment.create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
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
			renderpass_geometry[i].add_color_attachment(gbuffer[i].base_color_attachment.view);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].normal_attachment.view);
			renderpass_geometry[i].add_color_attachment(gbuffer[i].metalness_roughness_attachment.view);
			renderpass_geometry[i].add_depth_attachment(gbuffer[i].depth_attachment.view);
		}
	}

	void create_lighting_renderpass()
	{

	}

	void create_pipeline_gbuffer_pass()
	{
		VkFormat attachment_formats[]
		{
			gbuffer.base_color_attachment.info.imageFormat,
			gbuffer.normal_attachment.info.imageFormat,
			gbuffer.metalness_roughness_attachment.info.imageFormat,
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
		geometry_pass_pipeline.create_graphics(shader_geometry_pass, attachment_formats, gbuffer.depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, geometry_pass_pipeline.layout.vk_pipeline_layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void create_pipeline_lighting_pass()
	{
		shader_geometry_pass.create("fullscreen_quad_vert.vert.spv", "deferred_lighting_pass_frag.frag.spv");
	}

	/* Render pass writing geometry information to G-Buffers */
	void render_gbuffer_pass(VkCommandBuffer cmd_buffer)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Geometry Pass");
		renderpass_geometry[context.curr_frame_idx].begin(cmd_buffer, { render_size, render_size });
		/* .... */
		renderpass_geometry[context.curr_frame_idx].end(cmd_buffer);
	}

	/* Render pass using G-Buffers to compute lighting and render to fullscreen quad */
	void render_lighting_pass(VkCommandBuffer cmd_buffer)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Lighting Pass");
		renderpass_lighting[context.curr_frame_idx].begin(cmd_buffer, { context.swapchain->info.width , context.swapchain->info.height });
		/* .... */
		renderpass_lighting[context.curr_frame_idx].end(cmd_buffer);
	}

	void render(VkCommandBuffer cmd_buffer)
	{

	}

	void render(VkCommandBuffer cmd_buffer, const ObjectManager& object_manager)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Deferred Pass");


		render_lighting_pass(cmd_buffer);

		//draw_stats.reset();

		//vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		///* Frame level descriptor sets 1,2,3 */
		//vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx].vk_set, 0, nullptr);
		//vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set, 0, nullptr);
		//vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 2, 1, &descriptor_set.vk_set, 0, nullptr);

		//renderpass[context.curr_frame_idx].begin(cmd_buffer, { context.swapchain->info.width, context.swapchain->info.height });
		//for (size_t mesh_idx = 0; mesh_idx < object_manager.m_meshes.size(); mesh_idx++)
		//{
		//	/* Mesh descriptor set */
		//	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 3, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

		//	const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

		//	uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();
		//	forward_renderer_stats.increment_instance_count(instance_count);

		//	for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
		//	{
		//		const Primitive& p = mesh.geometry_data.primitives[prim_idx];
		//		forward_renderer_stats.increment_vertex_count(p.vertex_count * instance_count);

		//		pipeline.layout.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
		//		pipeline.layout.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

		//		vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
		//		forward_renderer_stats.increment_drawcall_count(1);
		//	}
		//}
		//renderpass[context.curr_frame_idx].end(cmd_buffer);
	}

	void update_shader_toggles()
	{
		
	}

	void show_ui() const override
	{

	}

	void reload_pipeline() override
	{
		vkDeviceWaitIdle(context.device);
	}

	VertexFragmentShader shader_geometry_pass;
	VertexFragmentShader shader_lighting_pass;

	VulkanRenderPassDynamic renderpass_geometry[NUM_FRAMES];
	VulkanRenderPassDynamic renderpass_lighting[NUM_FRAMES];

	VkFormat color_format;
	VkFormat depth_format;
	
	Pipeline geometry_pass_pipeline;
	VulkanRenderPassDynamic renderpass[NUM_FRAMES];

	VkDescriptorPool descriptor_pool;
	DescriptorSet descriptor_set;
	/* Contains different state toggles used inside fragment shader */
	struct ShaderToggles
	{
		
	} shader_params;

	DrawStatsEntry deferred_renderer_stats;
};