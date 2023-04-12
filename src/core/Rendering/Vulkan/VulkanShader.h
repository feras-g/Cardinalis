#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include <vulkan/vulkan.h>
#include <vector>

class VulkanShader
{
public:
	VulkanShader() = default;
	VulkanShader(const char* vertexShader, const char* fragShader);
	bool LoadModule(const VkShaderStageFlagBits stage, const char* filename);
	void load_from_file(const char* vertexShader, const char* fragShader);

	std::vector<VkPipelineShaderStageCreateInfo> pipelineStages;
protected:
	VkShaderModule fsModule;
	VkShaderModule vsModule;

};

#endif // !VULKAN_SHADER_H
