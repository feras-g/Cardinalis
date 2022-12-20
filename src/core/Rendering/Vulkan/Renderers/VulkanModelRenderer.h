#ifndef VULKAN_MODEL_RENDERER_H
#define VULKAN_MODEL_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanModelRenderer final : public VulkanRendererBase
{
public:
	VulkanModelRenderer() = default;
	explicit VulkanModelRenderer(const char* modelFilename, const char* textureFilename, uint32_t uniformDataSizeInBytes);
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) const override;

	virtual bool CreateRenderPass()   override;
	virtual bool CreateFramebuffers() override;

	~VulkanModelRenderer() final;
private:
	// Vertex + Index buffer data in storage buffer
	VkBuffer m_StorageBuffer; 
	size_t m_VtxBufferSizeInBytes;
	size_t m_IdxBufferSizeInBytes;

	VkSampler m_TextureSampler;
	VulkanTexture m_Texture;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H