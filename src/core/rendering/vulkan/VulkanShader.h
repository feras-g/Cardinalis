#pragma once

#include <vulkan/vulkan.hpp>
#include <set>
#include <unordered_map>

enum class ShaderType
{
	NONE = 0,
	COMPUTE = 1,
	VERTEX_FRAGMENT = 2
};

struct Shader
{
	bool create_shader_module(const VkShaderStageFlagBits stage, std::string_view filename, size_t& module_hash);
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	static bool compile(std::string_view shader_file);
};

struct VertexFragmentShader : Shader
{
	void create(const std::string& name, const std::string& vertex_shader_path, const std::string& fragment_shader_path);
	bool recreate_modules();
	bool compile();
	size_t hash_vertex_module;
	size_t hash_fragment_module;
	void destroy();

	std::string vertex_shader_filename; 
	std::string fragment_shader_filename;

	static constexpr ShaderType shader_type = ShaderType::VERTEX_FRAGMENT;
};

struct ComputeShader : Shader
{
	void create(const std::string& compute_shader_spirv_path);
	size_t hash_compute_module;
	void destroy();
	bool compile();
	bool recreate_module();
	std::string compute_shader_filename;

	static constexpr ShaderType shader_type = ShaderType::COMPUTE;
};

class ShaderLib
{
public:
	static ShaderLib* get()
	{
		if (s_shader_lib == nullptr)
		{
			s_shader_lib = new ShaderLib;
			return s_shader_lib;
		}

		return s_shader_lib;
	}

	Shader* get_shader(const std::string& name)
	{
		if (names.find(name) != names.end())
		{
			return &shaders[names.at(name)];
		}
		return nullptr;
	}

	void add_shader(const Shader& shader, const std::string& name)
	{
		size_t index = shaders.size();
		names.insert({ name, index });
		shaders.push_back(shader);
	}

	static inline std::unordered_map<std::string, size_t> names;
private:
	static inline ShaderLib* s_shader_lib = nullptr;
	std::vector<Shader> shaders;
};
