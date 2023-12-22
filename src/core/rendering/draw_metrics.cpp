#include "draw_metrics.h"

DrawMetricsEntry DrawMetricsManager::add_entry(const char* renderer_name)
{
	DrawMetricsEntry entry;

	entry.id = renderer_names.size();
	renderer_names.push_back(renderer_name);
	num_vertices.push_back(0);
	num_drawcalls.push_back(0);
	num_instances.push_back(0);

	return entry;
}

void DrawMetricsEntry::increment_drawcall_count(unsigned int count) 
{
	DrawMetricsManager::num_drawcalls[id] += count;
	DrawMetricsManager::total_drawcalls += count;
};

void DrawMetricsEntry::increment_vertex_count(unsigned int count)
{
	DrawMetricsManager::num_vertices[id] += count;
	DrawMetricsManager::total_vertices += count;
};

void DrawMetricsEntry::increment_instance_count(unsigned int count)
{
	DrawMetricsManager::num_instances[id] += count;
	DrawMetricsManager::total_instances += count;
};
