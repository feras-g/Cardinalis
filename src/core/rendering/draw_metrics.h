#pragma once

#include "core/engine/common.h"

struct DrawMetricsManager;

struct DrawMetricsEntry
{
	size_t id;

	void increment_drawcall_count(unsigned int count);
	void increment_vertex_count(unsigned int count);
	void increment_instance_count(unsigned int count);
};

struct DrawMetricsManager
{
	static inline std::vector<const char*> renderer_names;
	static inline std::vector<unsigned int> num_vertices;
	static inline std::vector<unsigned int> num_drawcalls;
	static inline std::vector<unsigned int> num_instances;

	static inline unsigned int total_drawcalls;
	static inline unsigned int total_vertices;
	static inline unsigned int total_instances;

	static DrawMetricsEntry add_entry(const char* renderer_name);

	static void reset()
	{
		total_drawcalls = 0;
		total_vertices = 0;
		total_instances = 0;

		memset(&num_vertices[0], 0, sizeof(unsigned int) * num_vertices.size());
		memset(&num_drawcalls[0], 0, sizeof(unsigned int) * num_drawcalls.size());
		memset(&num_instances[0], 0, sizeof(unsigned int) * num_instances.size());
	}
};

