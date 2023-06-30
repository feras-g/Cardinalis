#pragma once

#include <vulkan/vulkan.hpp>
#include <array>

struct Shader
{
	bool create_shader_module(const VkShaderStageFlagBits stage, const char* filename, VkShaderModule& out_module);
	std::vector<VkPipelineShaderStageCreateInfo> stages;
};

struct VertexFragmentShader : Shader
{
	void create(const char* vertex_shader_path, const char* fragment_shader_path);
	VkShaderModule module_vertex_stage = VK_NULL_HANDLE;
	VkShaderModule module_fragment_stage = VK_NULL_HANDLE;
};

struct ComputeShader : Shader
{
	void create(const char* compute_shader_path);
	VkShaderModule module_compute_stage = VK_NULL_HANDLE;
};