#include "VulkanShader.h"
#include "VulkanDebugUtils.h"
#include "core/engine/logger.h"
#include "core/rendering/vulkan/VkResourceManager.h"
#include "VulkanRenderInterface.h"

ShaderLib Shader::shader_library;

static constexpr uint32_t SPIRV_FOURCC = 0x07230203;

static std::string shader_src_folder("../../../data/shaders/vulkan/");
static std::string shader_spirv_folder("../../../data/shaders/vulkan/spirv/");
static std::string shader_header_folder("../../../data/shaders/vulkan/headers/");

static VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint)
{
	return VkPipelineShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr
	};
}

bool Shader::create_shader_module(const VkShaderStageFlagBits stage, std::string_view spirv_filename, size_t& module_hash)
{
	int supported = 
		int(VK_SHADER_STAGE_FRAGMENT_BIT) | int(VK_SHADER_STAGE_VERTEX_BIT) | int(VK_SHADER_STAGE_COMPUTE_BIT);

	if ((stage & supported) != stage)
	{
		LOG_ERROR("Shader stage not implemented.");
		return false;
	}

	FILE* fp = nullptr;
	errno_t fopenresult = fopen_s(&fp, (shader_spirv_folder + spirv_filename.data()).c_str(), "rb");
	
	if (fp == nullptr)
	{
		LOG_ERROR("Cannot open shader file {0} : {1}.", spirv_filename, strerror(fopenresult));
		assert(false);
	}

	uint32_t fourcc;
	fread(&fourcc, 4, 1, fp);
	if (fourcc != SPIRV_FOURCC)
	{
		LOG_ERROR("SPIR-V FourCC for {0} file is {1} -- should be {2}.", spirv_filename.data(), fourcc, SPIRV_FOURCC);
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
		LOG_ERROR("Cannot read SPIR-V bytecode.", spirv_filename.data());
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
	VK_CHECK(vkCreateShaderModule(ctx.device, &mci, nullptr, &out_module));

	stages.push_back(pipeline_shader_stage_create_info(out_module, stage, "main"));

	/* Add to resource manager */
	module_hash = VkResourceManager::get_instance(ctx.device)->add_shader_module(out_module);

	LOG_INFO("{1} : Created {0} module successfully.", vk_object_to_string(stage), spirv_filename);

	delete [] bytecode;

	return true;
}

static std::string get_shader_filename(const std::string& spirv_filename)
{
	return spirv_filename.substr(0, spirv_filename.find_last_of('.'));
}

void VertexFragmentShader::create(const std::string& vertex_shader_spirv_path, const std::string& fragment_shader_spirv_path)
{
	vertex_shader_filename = get_shader_filename(vertex_shader_spirv_path);
	fragment_shader_filename = get_shader_filename(fragment_shader_spirv_path);

	if (create_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_spirv_path.data(), hash_vertex_module))
	{
		shader_library.names.insert(vertex_shader_filename);
	}

	if (create_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_spirv_path.data(), hash_fragment_module))
	{
		shader_library.names.insert(fragment_shader_filename);
	}
}

bool VertexFragmentShader::compile()
{
	return Shader::compile(vertex_shader_filename) && 
		Shader::compile(fragment_shader_filename);
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

	std::vector<VkPipelineShaderStageCreateInfo> prev_stages = stages;

	std::string vertex_spirv_filename = vertex_shader_filename + ".spv";
	std::string fragment_spirv_filename = fragment_shader_filename + ".spv";


	stages.clear();
	if (create_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vertex_spirv_filename, hash_vertex_module) && create_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_spirv_filename, hash_fragment_module))
	{
		VkResourceManager::get_instance(ctx.device)->destroy_shader_module(prev_hash_vertex);
		VkResourceManager::get_instance(ctx.device)->destroy_shader_module(prev_hash_fragment);
		return true;
	}
	else
	{
		stages = prev_stages;
		hash_vertex_module = prev_hash_vertex;
		hash_fragment_module = prev_hash_fragment;
	}

	return false;
}

void VertexFragmentShader::destroy()
{
	stages.clear();
	VkResourceManager::get_instance(ctx.device)->destroy_shader_module(hash_vertex_module);
	VkResourceManager::get_instance(ctx.device)->destroy_shader_module(hash_fragment_module);
}

void ComputeShader::create(const std::string& compute_shader_spirv_path)
{
	create_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_spirv_path, hash_compute_module);
	compute_shader_filename = get_shader_filename(compute_shader_spirv_path);
}

void ComputeShader::destroy()
{
	VkResourceManager::get_instance(ctx.device)->destroy_shader_module(hash_compute_module);
}

bool ComputeShader::compile()
{
	return Shader::compile(compute_shader_filename);
}

bool ComputeShader::recreate_module()
{
	size_t prev_hash_compute_module = hash_compute_module;

	std::vector<VkPipelineShaderStageCreateInfo> prev_stages = stages;

	stages.clear();

	std::string spirv_filename = compute_shader_filename + ".spv";
	if (create_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, spirv_filename, hash_compute_module))
	{
		VkResourceManager::get_instance(ctx.device)->destroy_shader_module(prev_hash_compute_module);
		return true;
	}
	else
	{
		stages = prev_stages;
		hash_compute_module = prev_hash_compute_module;
	}

	return false;
}
