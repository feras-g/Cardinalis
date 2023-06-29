#include "VulkanShader.h"

#include "Core/EngineLogger.h"
#include "VulkanRenderInterface.h"

static constexpr uint32_t SPIRV_FOURCC = 0x07230203;

static std::string base_spirv_folder("../../../data/shaders/vulkan/spirv/");

VulkanShader::VulkanShader(const char* vertex_shader_path, const char* fragment_shader_path)
{
	load_from_file(vertex_shader_path, fragment_shader_path);
}

void VulkanShader::load_from_file(const char* vertex_shader_path, const char* fragment_shader_path)
{
	create_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_path);
	create_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_path);
}

void VulkanShader::load_from_file(const char* compute_shader_path)
{
	create_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_path);
}

bool VulkanShader::create_shader_module(const VkShaderStageFlagBits stage, const char* filename)
{
	int supported = 
		int(VK_SHADER_STAGE_FRAGMENT_BIT) | int(VK_SHADER_STAGE_VERTEX_BIT) | int(VK_SHADER_STAGE_COMPUTE_BIT);

	if ((stage & supported) != stage)
	{
		LOG_ERROR("Shader stage not implemented.");
		return false;
	}

	FILE* fp = nullptr;
	errno_t fopenresult = fopen_s(&fp, (base_spirv_folder + filename).c_str(), "rb");
	
	if (fp == nullptr)
	{
		LOG_ERROR("Cannot open shader file {0} : {1}.", filename, strerror(fopenresult));
		assert(false);
	}

	uint32_t fourcc;
	fread(&fourcc, 4, 1, fp);
	if (fourcc != SPIRV_FOURCC)
	{
		LOG_ERROR("SPIR-V FourCC for {0} file is {1} -- should be {2}.", filename, fourcc, SPIRV_FOURCC);
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
		LOG_ERROR("Cannot read SPIR-V bytecode.", filename);
		return false;
	}

	VkShaderModuleCreateInfo mci =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.flags = 0,
		.codeSize = bytecode_size_bytes,
		.pCode = bytecode,
	};

	VkShaderModule* module =
		stage == VK_SHADER_STAGE_VERTEX_BIT		? &module_fragment_stage :
		stage == VK_SHADER_STAGE_FRAGMENT_BIT  	? &module_vertex_stage :
		stage == VK_SHADER_STAGE_COMPUTE_BIT 	? &module_compute_stage : VK_NULL_HANDLE;

	VK_CHECK(vkCreateShaderModule(context.device, &mci, nullptr, module));

	pipeline_stages.push_back(PipelineShaderStageCreateInfo(*module, stage, "main"));

	LOG_INFO("{1} : Created {0} module successfully.", string_VkShaderStageFlagBits(stage), filename);

	delete [] bytecode;

	return true;
}
