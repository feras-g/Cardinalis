#pragma once

#include <vulkan/vulkan_core.h>
#include "core/rendering/vulkan/DescriptorSet.h"
#include "core/rendering/vulkan/VulkanShader.h"
#include "core/rendering/vulkan/RenderPass.h"

struct DrawStats;

struct IRenderer
{
	struct DrawStatsEntry
	{
		size_t renderer_id;
		void increment_drawcall_count(unsigned int count) {
			draw_stats.num_drawcalls[renderer_id] += count; 
			draw_stats.total_drawcalls += count;
		};

		void increment_vertex_count(unsigned int count) 
		{ 
			draw_stats.num_vertices[renderer_id] += count; 
			draw_stats.total_vertices += count;
		};

		void increment_instance_count(unsigned int count)
		{
			draw_stats.num_instances[renderer_id] += count;
			draw_stats.total_instances += count;
		};
	};

	struct DrawStats
	{
		std::vector<const char*> renderer_names;
		std::vector<unsigned int> num_vertices;
		std::vector<unsigned int> num_drawcalls;
		std::vector<unsigned int> num_instances;


		unsigned int total_drawcalls;
		unsigned int total_vertices;
		unsigned int total_instances;

		DrawStatsEntry add_entry(const char* renderer_name)
		{
			DrawStatsEntry entry;
			entry.renderer_id = renderer_names.size();

			renderer_names.push_back(renderer_name);
			num_vertices.push_back(0);
			num_drawcalls.push_back(0);
			num_instances.push_back(0);

			return entry;
		}

		void reset()
		{
			total_drawcalls = 0;
			total_vertices = 0;
			total_instances = 0;

			memset(&num_vertices[0], 0, sizeof(unsigned int) * num_vertices.size());
			memset(&num_drawcalls[0], 0, sizeof(unsigned int) * num_drawcalls.size());
			memset(&num_instances[0], 0, sizeof(unsigned int) * num_instances.size());
		}
	};

	static inline DrawStats draw_stats;

	/* Call all create* functions */
	virtual void init() = 0;
	virtual void create_pipeline() = 0;
	virtual void create_renderpass() = 0;

	virtual void render(VkCommandBuffer cmd_buffer) = 0;

	virtual void on_window_resize()
	{
		create_renderpass();
	}

	/* Add UI elements to be displayed */
	virtual void show_ui() const = 0;

	Pipeline pipeline;
	VertexFragmentShader shader;
	VkFormat color_format;
	VkFormat depth_format;
	VulkanRenderPassDynamic renderpass[NUM_FRAMES];
	DrawStatsEntry renderer_stats;
};
