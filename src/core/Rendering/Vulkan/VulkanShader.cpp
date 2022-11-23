#include "VulkanShader.h"

#include "Core/EngineLogger.h"
#include "VulkanRenderInterface.h"

#define SPIRV_FOURCC 0x07230203

static const char* ShaderStageToString(ShaderStage stage);
static std::string baseSpirvFolder("../../../data/shaders/vulkan/spirv/");
VulkanShader::VulkanShader(const char* vertexShader, const char* fragShader)
{
	LoadModule(ShaderStage::VERTEX_STAGE, vertexShader);
	LoadModule(ShaderStage::FRAGMENT_STAGE, fragShader);
}
void VulkanShader::LoadModule(ShaderStage stage, const char* filename)
{
	FILE* fp;
	errno_t fopenresult = fopen_s(&fp, (baseSpirvFolder + filename).c_str(), "rb");
	
	if (fp == nullptr)
	{
		LOG_ERROR("Cannot open shader file {0} : {1}.", filename, strerror(fopenresult));
		return;
	}

	uint32_t fourcc;
	fread(&fourcc, 4, 1, fp);
	if (fourcc != SPIRV_FOURCC)
	{
		LOG_ERROR("SPIR-V FourCC for {0} file is {1} -- should be {2}.", filename, fourcc, SPIRV_FOURCC);
		return;
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
		return;
	}

	VkShaderModuleCreateInfo mci =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.flags = 0,
		.codeSize = filesizeInBytes,
		.pCode = bytecode,
	};

	VK_CHECK(vkCreateShaderModule(context.device, &mci, nullptr,
		stage == ShaderStage::VERTEX_STAGE   ? &fsModule :
		stage == ShaderStage::FRAGMENT_STAGE ? &vsModule : VK_NULL_HANDLE));

	LOG_INFO("{1} : Created {0} module successfully.", ShaderStageToString(stage), filename);

	delete [] bytecode;
}

const char* ShaderStageToString(ShaderStage stage)
{
	switch (stage)
	{
	case ShaderStage::VERTEX_STAGE:
		return "vertex stage";
		break;
	case ShaderStage::FRAGMENT_STAGE:
		return "fragment stage";
		break;
	default:
		break;
	}
}
