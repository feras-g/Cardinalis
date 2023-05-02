#include "VulkanShader.h"

#include "Core/EngineLogger.h"
#include "VulkanRenderInterface.h"

#define SPIRV_FOURCC 0x07230203

static std::string base_spirv_folder("../../../data/shaders/vulkan/spirv/");
VulkanShader::VulkanShader(const char* vertexShader, const char* fragShader)
{
	load_from_file(vertexShader, fragShader);
}

void VulkanShader::load_from_file(const char* vertexShader, const char* fragShader)
{
	LoadModule(VK_SHADER_STAGE_VERTEX_BIT, vertexShader);
	LoadModule(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader);
}

bool VulkanShader::LoadModule(const VkShaderStageFlagBits stage, const char* filename)
{
	if (stage != VK_SHADER_STAGE_FRAGMENT_BIT && stage != VK_SHADER_STAGE_VERTEX_BIT)
	{
		LOG_ERROR("Shader stage not implemented.");
		return false;
	}

	FILE* fp;
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

	// Get file size in bytes
	fseek(fp, 0L, SEEK_END);
	size_t filesizeInBytes = ftell(fp);
	rewind(fp);

	// Extract bytecode
	uint32_t* bytecode = new uint32_t[filesizeInBytes/sizeof(uint32_t)];
	fread(bytecode, filesizeInBytes, 1, fp);
	fclose(fp);

	// Create module
	if (!bytecode)
	{
		LOG_ERROR("Cannot read SPIR-V bytecode.", filename);
		return false;
	}

	VkShaderModuleCreateInfo mci =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.flags = 0,
		.codeSize = filesizeInBytes,
		.pCode = bytecode,
	};

	VkShaderModule* module = stage == VK_SHADER_STAGE_VERTEX_BIT	? &fsModule :
							 stage == VK_SHADER_STAGE_FRAGMENT_BIT  ? &vsModule : VK_NULL_HANDLE;

	VK_CHECK(vkCreateShaderModule(context.device, &mci, nullptr, module));

	pipelineStages.push_back(PipelineShaderStageCreateInfo(*module, stage, "main"));

	LOG_INFO("{1} : Created {0} module successfully.", string_VkShaderStageFlagBits(stage), filename);

	delete [] bytecode;

	return true;
}
