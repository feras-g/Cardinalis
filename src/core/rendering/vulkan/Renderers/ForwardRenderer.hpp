#pragma once

#include "DebugLineRenderer.hpp"

struct ForwardRenderer
{
	void init()
	{
		create_renderpass();
		create_pipeline();
	}

	void create_pipeline()
	{
		ssbo_shader_toggles.init(Buffer::Type::STORAGE, sizeof(ShaderParams), "ForwardRenderer Shader Toggles");
		ssbo_shader_toggles.create();
		ssbo_shader_toggles.upload(context.device, &shader_params, 0, sizeof(ShaderParams));

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
			ObjectManager::get_instance().mesh_descriptor_set_layout, 
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			ObjectManager::get_instance().m_descriptor_set_bindless_textures.layout.vk_set_layout,
			descriptor_set.layout.vk_set_layout
		};

		pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
		pipeline.layout.add_push_constant_range("Material", { .stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(Material) });

		pipeline.layout.create(layouts);
		pipeline.create_graphics_pipeline_dynamic(shader, std::span<VkFormat>(&color_format, 1), depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void create_renderpass()
	{
		color_format = context.swapchain->color_attachments[0].info.imageFormat;
		depth_format = context.swapchain->depth_attachments[0].info.imageFormat;

		for (uint32_t i = 0; i < NUM_FRAMES; i++)
		{
			renderpass[i].reset();
			renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
			renderpass[i].add_depth_attachment(context.swapchain->depth_attachments[i].view, VK_ATTACHMENT_LOAD_OP_CLEAR);
		}
	}

	void render(VkCommandBuffer cmd_buffer, const ObjectManager& object_manager)
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Forward Pass");

		
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		/* Frame level descriptor sets 1,2,3 */
		VkDescriptorSet frame_sets[3]
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx].vk_set,
			ObjectManager::get_instance().m_descriptor_set_bindless_textures.vk_set,
			descriptor_set.vk_set
		};
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 3, frame_sets, 0, nullptr);

		renderpass[context.curr_frame_idx].begin(cmd_buffer, { context.swapchain->info.width, context.swapchain->info.height });
		for (size_t mesh_idx = 0; mesh_idx < object_manager.m_meshes.size(); mesh_idx++)
		{
			/* Mesh descriptor set */
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &object_manager.m_descriptor_sets[mesh_idx].vk_set, 0, nullptr);

			const VulkanMesh& mesh = object_manager.m_meshes[mesh_idx];

			uint32_t instance_count = (uint32_t)object_manager.m_mesh_instance_data[mesh_idx].size();

			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = mesh.geometry_data.primitives[prim_idx];

				pipeline.layout.cmd_push_constants(cmd_buffer, "Material", &object_manager.m_materials[p.material_id]);
				pipeline.layout.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);


				vkCmdDraw(cmd_buffer, p.index_count, instance_count, p.first_index, 0);


			}
		}
		renderpass[context.curr_frame_idx].end(cmd_buffer);
	}

	void on_window_resize()
	{
		create_renderpass();
	}

	VkFormat color_format;
	VkFormat depth_format;
	VertexFragmentShader shader;
	Pipeline pipeline;
	VulkanRenderPassDynamic renderpass[NUM_FRAMES];

	VkDescriptorPool descriptor_pool;
	DescriptorSet descriptor_set;
	/* Contains different state toggles used inside fragment shader */
	struct ShaderParams
	{
		unsigned a;
	} shader_params;

	Buffer ssbo_shader_toggles;

	DebugLineRenderer* p_debug_line_renderer = nullptr;
};