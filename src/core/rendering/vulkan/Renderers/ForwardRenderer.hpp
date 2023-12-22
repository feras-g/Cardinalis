#pragma once

#include "DebugLineRenderer.hpp"
#include "core/rendering/vulkan/VulkanUI.h"

struct ForwardRenderer : public IRenderer
{
	void init()
	{
		name = "Forward Renderer";
		create_renderpass();
		create_pipeline();
		draw_metrics = DrawMetricsManager::add_entry(name.c_str());
	}

	void create_pipeline()
	{
		ssbo_shader_toggles.init(vk::buffer::type::STORAGE, sizeof(ShaderToggles), "ForwardRenderer Shader Toggles");
		ssbo_shader_toggles.create();
		update_shader_toggles();

		std::array<VkDescriptorPoolSize, 1> pool_sizes
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

		descriptor_set.layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, "ForwardRenderer Shader Params");
		descriptor_set.layout.create("ForwardRenderer Shader Params Layout");
		descriptor_set.create(descriptor_pool, "ForwardRenderer");

		descriptor_set.write_descriptor_storage_buffer(0, ssbo_shader_toggles, 0, VK_WHOLE_SIZE);

		shader.create("instanced_mesh_vert.vert.spv", "forward_frag.frag.spv");

		std::vector<VkDescriptorSetLayout> layouts = 
		{ 
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			ObjectManager::get_instance().m_descriptor_set_bindless_textures.layout.vk_set_layout,
			ObjectManager::get_instance().mesh_descriptor_set_layout, 
			descriptor_set.layout.vk_set_layout,
		};

		pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
		pipeline.layout.add_push_constant_range("Material", { .stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(Material) });

		pipeline.layout.create(layouts);
		pipeline.create_graphics(shader, std::span<VkFormat>(&color_format, 1), depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void create_renderpass()
	{
		color_format = ctx.swapchain->color_attachments[0].info.imageFormat;
		depth_format = ctx.swapchain->depth_attachments[0].info.imageFormat;

		for (uint32_t i = 0; i < NUM_FRAMES; i++)
		{
			renderpass[i].reset();
			renderpass[i].add_color_attachment(ctx.swapchain->color_attachments[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
			renderpass[i].add_depth_attachment(ctx.swapchain->depth_attachments[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		}
	}

	void render(VkCommandBuffer cmd_buffer)
	{
		
	}

	void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Forward Pass");

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		/* Frame level descriptor sets 1,2,3 */
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[ctx.curr_frame_idx].vk_set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 3, 1, &descriptor_set.vk_set, 0, nullptr);

		renderpass[ctx.curr_frame_idx].begin(cmd_buffer, { ctx.swapchain->info.width, ctx.swapchain->info.height });

		const ObjectManager& object_manager = ObjectManager::get_instance();
		for (size_t mesh_idx : mesh_list)
		{
			/* Mesh descriptor set */
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 2, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

			const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

			uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();
			draw_metrics.increment_instance_count(instance_count);

			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = mesh.geometry_data.primitives[prim_idx];
				draw_metrics.increment_vertex_count(p.vertex_count * instance_count);

				pipeline.layout.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
				pipeline.layout.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

				vkCmdDraw(cmd_buffer, p.vertex_count, instance_count, p.first_vertex, 0);
				draw_metrics.increment_drawcall_count(1);
			}
		}
		renderpass[ctx.curr_frame_idx].end(cmd_buffer);
	}

	void update_shader_toggles()
	{
		ssbo_shader_toggles.upload(ctx.device, &shader_params, 0, sizeof(ShaderToggles));
	}

	void show_ui() override
	{

	}

	bool reload_pipeline() override
	{
		vkDeviceWaitIdle(ctx.device);
		return pipeline.reload_pipeline();
	}

	VkFormat color_format;
	VkFormat depth_format;
	VertexFragmentShader shader;
	Pipeline pipeline;
	vk::renderpass_dynamic renderpass[NUM_FRAMES];

	VkDescriptorPool descriptor_pool;
	vk::descriptor_set descriptor_set;
	/* Contains different state toggles used inside fragment shader */
	struct ShaderToggles
	{
		bool enable_normal_mapping;
	} shader_params;

	vk::buffer ssbo_shader_toggles;

	DebugLineRenderer* p_debug_line_renderer = nullptr;

	DrawMetricsEntry draw_metrics;
};