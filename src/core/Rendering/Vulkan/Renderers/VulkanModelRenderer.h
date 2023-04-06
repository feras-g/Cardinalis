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

	bool Init();
	void draw_scene(size_t currentImageIdx, VkCommandBuffer cmdBuffer);
	bool UpdateBuffers(size_t currentImage, glm::mat4 model, glm::mat4 view, glm::mat4 proj);
	bool UpdateDescriptorSets(VkDevice device) final override;
	bool CreateAttachments();

	// Offscreen images

	std::vector<Texture2D> m_ColorAttachments; // 0: Color, 1: Normal
	std::vector<Texture2D> m_DepthAttachments;

	~VulkanModelRenderer() final;
private:
	VulkanModel m_Model;
	std::unique_ptr<VulkanShader> m_Shader;

	std::vector<VkFormat> m_ColorAttachmentFormats = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R16G16B16A16_SFLOAT };
	VkFormat m_DepthAttachmentFormat	= VK_FORMAT_D32_SFLOAT_S8_UINT;

	VkSampler m_TextureSampler;
	Texture2D m_Texture;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H