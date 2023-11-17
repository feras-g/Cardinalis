#include "VulkanShader.h"
#include "VulkanDebugUtils.h"
#include "core/engine/EngineLogger.h"
#include "core/rendering/vulkan/VkResourceManager.h"
#include "VulkanRenderInterface.h"

ShaderLib Shader::shader_library;

static constexpr uint32_t SPIRV_FOURCC = 0x07230203;

static std::string shader_src_folder("../../../data/shaders/vulkan/");
static std::string shader_spirv_folder("../../../data/shaders/vulkan/spirv/");
static std::string shader_header_folder("../../../data/shaders/vulkan/headers/");

bool Shader::create_shader_module(const VkShaderStageFlagBits stage, std::string_view filename, size_t& module_hash)
{
	int supported = 
		int(VK_SHADER_STAGE_FRAGMENT_BIT) | int(VK_SHADER_STAGE_VERTEX_BIT) | int(VK_SHADER_STAGE_COMPUTE_BIT);

	if ((stage & supported) != stage)
	{
		LOG_ERROR("Shader stage not implemented.");
		return false;
	}

	FILE* fp = nullptr;
	errno_t fopenresult = fopen_s(&fp, (shader_spirv_folder + filename.data()).c_str(), "rb");
	
	if (fp == nullptr)
	{
		LOG_ERROR("Cannot open shader file {0} : {1}.", filename, strerror(fopenresult));
		assert(false);
	}

	uint32_t fourcc;
	fread(&fourcc, 4, 1, fp);
	if (fourcc != SPIRV_FOURCC)
	{
		LOG_ERROR("SPIR-V FourCC for {0} file is {1} -- should be {2}.", filename.data(), fourcc, SPIRV_FOURCC);
		assert(false);
	}

	/* Extract bytecode */
	fseek(fp, 0L, SEEK_END);
	size_t bytecode_size_bytes = ftell(fp);
	rewind(fp);

	uint32_t* bytecode = new uint32_t[bytecode_size_bytes/sizeof(uint32_t)];
	fread(bytecode, bytecode_size_bytes, 1, fp);
	fclose(fp);

	/* Create module */
	if (bytecode == nullptr)
	{
		LOG_ERROR("Cannot read SPIR-V bytecode.", filename.data());
		return false;
	}

	VkShaderModuleCreateInfo mci =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.flags = 0,
		.codeSize = bytecode_size_bytes,
		.pCode = bytecode,
	};

	VkShaderModule out_module = VK_NULL_HANDLE;
	VK_CHECK(vkCreateShaderModule(context.device, &mci, nullptr, &out_module));

	stages.push_back(PipelineShaderStageCreateInfo(out_module, stage, "main"));

	/* Add to resource manager */
	module_hash = VkResourceManager::get_instance(context.device)->add_shader_module(out_module);

	LOG_INFO("{1} : Created {0} module successfully.", vk_object_to_string(stage), filename);

	delete [] bytecode;

	return true;
}

void VertexFragmentShader::create(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
{
	this->vertex_shader_spirv_filename = vertex_shader_path.data();
	this->fragment_shader_spirv_filename = fragment_shader_path.data();

	if (create_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_path.data(), hash_vertex_module))
	{
		shader_library.names.insert(vertex_shader_path.substr(0, vertex_shader_path.find_last_of('.')));
	}

	if (create_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_path.data(), hash_fragment_module))
	{
		shader_library.names.insert(fragment_shader_path.substr(0, fragment_shader_path.find_last_of('.')).data());
	}
}

bool VertexFragmentShader::compile()
{
	return Shader::compile(vertex_shader_spirv_filename.substr(0, vertex_shader_spirv_filename.find_last_of('.'))) && Shader::compile(fragment_shader_spirv_filename.substr(0, fragment_shader_spirv_filename.find_last_of('.')));
}

bool Shader::compile(std::string_view shader_file)
{
	char command[1500];

	std::string src_path = shader_src_folder + shader_file.data();
	std::string dst_path = shader_spirv_folder + shader_file.data() + ".spv";
	std::string header_path = shader_header_folder;

	sprintf(command, "%s\\Bin\\glslc.exe %s -g -I %s -o %s", std::getenv("VULKAN_SDK"), src_path.c_str(), header_path.c_str(), dst_path.c_str());

	bool result = system(command);

	return result == 0;
}

bool VertexFragmentShader::recreate_modules()
{
	size_t prev_hash_vertex = hash_vertex_module;
	size_t prev_hash_fragment = hash_fragment_module;

	size_t hash_vertex;
	size_t hash_fragment;

	std::vector<VkPipelineShaderStageCreateInfo> prev_stages = stages;

	stages.clear();
	if (create_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_spirv_filename, hash_vertex) && create_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_spirv_filename, hash_fragment))
	{
		hash_vertex_module = hash_vertex;
		hash_fragment_module = hash_fragment;

		VkResourceManager::get_instance(context.device)->destroy_shader_module(prev_hash_vertex);
		VkResourceManager::get_instance(context.device)->destroy_shader_module(prev_hash_fragment);

		return true;
	}
	else
	{
		stages = prev_stages;
	}

	return false;
}

void VertexFragmentShader::destroy()
{
	stages.clear();
	VkResourceManager::get_instance(context.device)->destroy_shader_module(hash_vertex_module);
	VkResourceManager::get_instance(context.device)->destroy_shader_module(hash_fragment_module);
}

void ComputeShader::create(const char* compute_shader_path)
{
	create_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_path, hash_compute_module);
}

void ComputeShader::destroy()
{
	VkResourceManager::get_instance(context.device)->destroy_shader_module(hash_compute_module);
}

bool ComputeShader::recompile()
{
	return false;
}
