#pragma once

#include <vulkan/vulkan.hpp>
#include <array>

struct Shader
{
	bool create_shader_module(const VkShaderStageFlagBits stage, const char* filename, size_t& module_hash);
	std::vector<VkPipelineShaderStageCreateInfo> stages;
};

struct VertexFragmentShader : Shader
{
	void create(const char* vertex_shader_path, const char* fragment_shader_path);
	size_t hash_vertex_module;
	size_t hash_fragment_module;
	void destroy();
};

struct ComputeShader : Shader
{
	void create(const char* compute_shader_path);
	size_t hash_compute_module;
	void destroy();
};