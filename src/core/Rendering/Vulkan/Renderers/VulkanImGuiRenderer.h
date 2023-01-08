#ifndef VULKAN_IMGUI_RENDERER_H
#define VULKAN_IMGUI_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

struct ImDrawData;
struct ImGuiIO;
class VulkanShader;

class VulkanImGuiRenderer : public VulkanRendererBase
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void Initialize(const std::vector<VulkanTexture>& textures);
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) override;
	void UpdateBuffers(size_t currentImage, ImDrawData* pDrawData);
	void LoadSceneViewTextures(VulkanTexture* modelRendererColor, VulkanTexture* modelRendererDepth);

	// Texture Ids
	uint32_t m_ModelRendererColorTextureId[NUM_FRAMES];
	uint32_t m_ModelRendererDepthTextureId[NUM_FRAMES];

	float m_SceneViewAspectRatio = 1.0;
	bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts) override;

	~VulkanImGuiRenderer() final;
private:
	ImDrawData* m_pDrawData = nullptr;

	VkSampler m_SamplerRepeatLinear;
	VulkanTexture m_FontTexture;
	std::vector<VulkanTexture> m_Textures;	// Textures displayed inside UI
	std::vector<VkDescriptorImageInfo> m_TextureDescriptors;

	// Storage buffers
	VkBuffer m_StorageBuffers[NUM_FRAMES];
	VkDeviceMemory m_StorageBufferMems[NUM_FRAMES];

	std::unique_ptr<VulkanShader> m_Shader;

	bool CreateFontTexture(ImGuiIO* io, const char* fontPath, VulkanTexture& out_Font);
	bool CreatePipeline(VkDevice device);
	bool CreateUniformBuffers(size_t dataSizeInBytes);

	bool UpdateDescriptorSets(VkDevice device) final override;

	virtual bool CreateRenderPass() override;
	virtual bool CreateFramebuffers() override;

	VkDescriptorPool m_ImGuiDescriptorPool;

};
#endif // !VULKAN_IMGUI_RENDERER_H
