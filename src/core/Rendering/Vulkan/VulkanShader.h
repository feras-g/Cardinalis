#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include <vulkan/vulkan.h>

enum class ShaderStage { VERTEX_STAGE = 0, FRAGMENT_STAGE = 1 };

class VulkanShader
{
public:
	VulkanShader(const char* vertexShader, const char* fragShader);
	void LoadModule(ShaderStage stage, const char* filename);

protected:
	VkShaderModule fsModule;
	VkShaderModule vsModule;
};

#endif // !VULKAN_SHADER_H
