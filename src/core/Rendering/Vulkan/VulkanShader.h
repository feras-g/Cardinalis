#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanShader
{
public:
	VulkanShader() = default;
	VulkanShader(const char* vertex_shader_path, const char* fragment_shader_path);
	VulkanShader(const char* compute_shader_path);
	bool create_shader_module(const VkShaderStageFlagBits stage, const char* filename);
	void load_from_file(const char* vertex_shader_path, const char* fragment_shader_path);
	void load_from_file(const char* compute_shader_path);

	std::vector<VkPipelineShaderStageCreateInfo> pipeline_stages;
protected:
	VkShaderModule module_fragment_stage = VK_NULL_HANDLE;
	VkShaderModule module_vertex_stage = VK_NULL_HANDLE;
	VkShaderModule module_compute_stage = VK_NULL_HANDLE;
};

