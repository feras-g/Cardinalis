#ifndef VULKAN_MODEL_RENDERER_H
#define VULKAN_MODEL_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanModel.h"

class VulkanShader;

class VulkanModelRenderer final : public VulkanRendererBase
{
public:
	VulkanModelRenderer() = default;
	explicit VulkanModelRenderer(const char* modelFilename, const char* textureFilename);
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer)  override;

	bool CreatePipeline();
	bool UpdateBuffers(size_t currentImage, glm::mat4 model, glm::mat4 view, glm::mat4 proj);
	bool UpdateDescriptorSets(VkDevice device) final override;
	virtual bool CreateRenderPass()   override;
	virtual bool CreateFramebuffers() override;


	~VulkanModelRenderer() final;
private:
	VulkanModel m_Model;
	std::unique_ptr<VulkanShader> m_Shader;

	VkSampler m_TextureSampler;
	VulkanTexture m_Texture;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H