#pragma once

#include <vulkan/vulkan.hpp>
#include <set>

struct ShaderLib
{
	std::set<std::string> names;
};


struct Shader
{
	static ShaderLib shader_library;
	bool create_shader_module(const VkShaderStageFlagBits stage, std::string_view filename, size_t& module_hash);
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	static bool compile(std::string_view shader_file);
};

struct VertexFragmentShader : Shader
{
	void create(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
	bool recreate_modules();
	bool compile();
	size_t hash_vertex_module;
	size_t hash_fragment_module;
	void destroy();

	std::string vertex_shader_filename; 
	std::string fragment_shader_filename;
};

struct ComputeShader : Shader
{
	void create(const std::string& compute_shader_spirv_path);
	size_t hash_compute_module;
	void destroy();
	bool compile();
	bool recreate_module();
	std::string compute_shader_filename;
};