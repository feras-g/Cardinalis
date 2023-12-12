#pragma once

#include "IRenderer.h"

struct DebugLineRenderer : public IRenderer
{
	void init() override
	{
		name = "Debug Line Renderer";
		create_renderpass();
		create_pipeline();
	}

	void create_pipeline() override
	{
		vertex_buffer.init(Buffer::Type::STORAGE, vtx_buffer_max_size, "DebugLineRenderer Vertex Buffer");
		vertex_buffer.create();

		Buffer staging;
		staging.init(Buffer::Type::STAGING, sizeof(VkDrawIndirectCommand), "");
		staging.create();

		VkDrawIndirectCommand init_indirect_cmd{0, 1, 0, 0};
		staging.upload(context.device, &init_indirect_cmd, 0, sizeof(VkDrawIndirectCommand));

		indirect_cmd_buffer.init(Buffer::Type::INDIRECT, sizeof(VkDrawIndirectCommand), "DebugLineRenderer Indirect Command"); /* Device local */
		indirect_cmd_buffer.create();
		copy_from_buffer(staging, indirect_cmd_buffer, sizeof(VkDrawIndirectCommand));

		std::array<VkDescriptorPoolSize, 1> pool_sizes
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

		debug_line_descriptor_set.layout.add_storage_buffer_binding(0, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "DebugLineRenderer Vertex Buffer Binding");
		debug_line_descriptor_set.layout.add_storage_buffer_binding(1, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "DebugLineRenderer Indirect Command Binding");

		debug_line_descriptor_set.layout.create("DebugLineRenderer Descriptor Layout");
		debug_line_descriptor_set.create(descriptor_pool, "DebugLineRenderer Descriptor Set");

		debug_line_descriptor_set.write_descriptor_storage_buffer(0, vertex_buffer, 0, VK_WHOLE_SIZE);
		debug_line_descriptor_set.write_descriptor_storage_buffer(1, indirect_cmd_buffer, 0, VK_WHOLE_SIZE);

		render_shader.create("debug_line_rendering_vert.vert.spv", "debug_line_rendering_frag.frag.spv");

		VkDescriptorSetLayout graphics_pipeline_layouts[] =
		{
			debug_line_descriptor_set.layout,
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
		};

		graphics_pipeline.layout.create(graphics_pipeline_layouts);
		graphics_pipeline.create_graphics(render_shader, std::span<VkFormat>(&color_format, 1), depth_format, Pipeline::Flags::ENABLE_DEPTH_STATE, graphics_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

		VkDescriptorSetLayout compute_pipeline_layouts[] =
		{
			debug_line_descriptor_set.layout
		};
		populate_shader.create("debug_line_populate_comp.comp.spv");
		compute_pipeline.create_compute(populate_shader, compute_pipeline_layouts);
	}

	void create_renderpass() override
	{
		color_format = context.swapchain->color_attachments[0].info.imageFormat;
		depth_format = context.swapchain->depth_attachments[0].info.imageFormat;

		for (uint32_t i = 0; i < NUM_FRAMES; i++)
		{
			renderpass[i].reset();
			renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
			renderpass[i].add_depth_attachment(context.swapchain->depth_attachments[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
		}
	}

	void render(VkCommandBuffer cmd_buffer) override
	{
		/* Dispatch compute shader populating vertex buffer */
		if (!vtx_buffer_populated)
		{
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, graphics_pipeline.layout, 0, 1, debug_line_descriptor_set, 0, nullptr);
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline);
			vkCmdDispatch(cmd_buffer, 1, 1, 1);
			vtx_buffer_populated = true;
		}

		renderpass[context.curr_frame_idx].begin(cmd_buffer, { context.swapchain->info.width, context.swapchain->info.height });
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 1, debug_line_descriptor_set, 0, nullptr);
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 1, 1, &VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx].vk_set, 0, nullptr);
		vkCmdDrawIndirect(cmd_buffer, indirect_cmd_buffer, 0, 1, 0);
		renderpass[context.curr_frame_idx].end(cmd_buffer);
	}

	void on_window_resize()
	{
		create_renderpass();
	}

	virtual void show_ui()
	{

	}

	virtual bool reload_pipeline()
	{
		return false;
	}

	struct DebugPoint
	{
		float position[4];
		float color[4];
	};

	VkFormat color_format;
	VkFormat depth_format;

	/* Compute shader used to populate the vertex buffer with points */
	Pipeline compute_pipeline;
	ComputeShader populate_shader;
	bool vtx_buffer_populated = false;
	/* Vertex/Fragment shaders to render the points */
	VertexFragmentShader render_shader;
	VkDrawIndirectCommand command;
	VkDescriptorPool descriptor_pool;
	DescriptorSet debug_line_descriptor_set;
	Buffer vertex_buffer;
	Buffer indirect_cmd_buffer;
	Pipeline graphics_pipeline;
	VulkanRenderPassDynamic renderpass[NUM_FRAMES];

	size_t max_lines  = 16384;
	size_t vtx_buffer_max_size = 2 * sizeof(DebugPoint) * max_lines;
};