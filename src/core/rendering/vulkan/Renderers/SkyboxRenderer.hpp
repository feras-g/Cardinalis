#pragma once

#include "IRenderer.h"
#include "core/rendering/vulkan/VulkanUI.h"

struct SkyboxRenderer : public IRenderer
{
	void init() override
	{

	}

	void init(const Texture2D& cubemap)
	{
		/* Create descriptor set containing the cubemap */
		VkSampler sampler_clamp_linear = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		VkDescriptorPoolSize pool_sizes[1]
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		env_map_descriptor_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Skybox Cubemap");
		env_map_descriptor_set.layout.create("Skybox Cubemap Desc set layout");
		env_map_descriptor_set.create(descriptor_pool, "Skybox");
		env_map_descriptor_set.write_descriptor_combined_image_sampler(0, cubemap.view, sampler_clamp_linear);


		init_assets();
		create_renderpass();
		create_pipeline();
	}

	void init_prefiltered_env_map()
	{

	}

	void init_assets()
	{
		/* Skybox */
		id_mesh_skybox = ObjectManager::get_instance().m_mesh_id_from_name["Mesh_Skybox"];
	}


	void create_pipeline() override
	{
		shader.create("skybox_vert.vert.spv", "single_cubemap_input_attachment_frag.frag.spv");

		VkDescriptorSetLayout layouts[]
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			ObjectManager::get_instance().mesh_descriptor_set_layout,
			env_map_descriptor_set.layout,
		};

		pipeline.layout.add_push_constant_range("Primitive Model Matrix", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
		pipeline.layout.create(layouts);
		pipeline.create_graphics(shader, std::span<VkFormat>(&color_format, 1), depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	}

	void render(VkCommandBuffer cmd_buffer) override
	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Skybox Pass");
		glm::vec2 render_size = { DeferredRenderer::gbuffer[0].final_lighting.info.width, DeferredRenderer::gbuffer[0].final_lighting.info.height };

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		set_viewport_scissor(cmd_buffer, render_size.x, render_size.y, true);

		VkDescriptorSet descriptor_sets[]
		{
			VulkanRendererCommon::get_instance().m_framedata_desc_set[ctx.curr_frame_idx].vk_set,
			ObjectManager::get_instance().m_descriptor_sets[id_mesh_skybox],
			env_map_descriptor_set
		};

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 3, descriptor_sets, 0, nullptr);
		renderpass[ctx.curr_frame_idx].begin(cmd_buffer, render_size);

		const ObjectManager& object_manager = ObjectManager::get_instance();
		for (int prim_idx = 0; prim_idx < object_manager.m_meshes[id_mesh_skybox].geometry_data.primitives.size(); prim_idx++)
		{
			const Primitive& p = object_manager.m_meshes[id_mesh_skybox].geometry_data.primitives[prim_idx];
			pipeline.layout.cmd_push_constants(cmd_buffer, "Primitive Model Matrix", &p.model);

			vkCmdDraw(cmd_buffer, p.vertex_count, 1, p.first_vertex, 0);
		}

		renderpass[ctx.curr_frame_idx].end(cmd_buffer);
	}

	void show_ui() override
	{

	}

	bool reload_pipeline() override { return false; }

	void create_renderpass() override
	{
		color_format = DeferredRenderer::gbuffer[0].final_lighting.info.imageFormat;
		depth_format = DeferredRenderer::gbuffer[0].depth_attachment.info.imageFormat;
		for (int i = 0; i < NUM_FRAMES; i++)
		{
			renderpass[i].reset();
			renderpass[i].add_color_attachment(DeferredRenderer::gbuffer[i].final_lighting.view, VK_ATTACHMENT_LOAD_OP_LOAD);
			renderpass[i].add_depth_attachment(DeferredRenderer::gbuffer[i].depth_attachment.view, VK_ATTACHMENT_LOAD_OP_LOAD);
		}
	}

	vk::descriptor_set env_map_descriptor_set;
	size_t id_mesh_skybox;
	VkDescriptorPool descriptor_pool;
	Texture2D* env_map_texture_handle;
};
