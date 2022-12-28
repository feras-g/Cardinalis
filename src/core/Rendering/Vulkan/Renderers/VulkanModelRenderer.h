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

	// Use offscreen buffer to render into
	std::vector<VulkanTexture> m_ColorAttachments;
	std::vector<VulkanTexture> m_DepthStencilAttachments;

	~VulkanModelRenderer() final;
private:
	VulkanModel m_Model;
	std::unique_ptr<VulkanShader> m_Shader;



	VkFormat m_ColorFormat        = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat m_DepthStencilFormat = VK_FORMAT_D32_SFLOAT;

	VkSampler m_TextureSampler;
	VulkanTexture m_Texture;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H