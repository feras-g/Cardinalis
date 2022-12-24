#ifndef VULKAN_IMGUI_RENDERER_H
#define VULKAN_IMGUI_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"

struct ImDrawData;
struct ImGuiIO;
class VulkanShader;

class VulkanImGuiRenderer final : public VulkanRendererBase
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void Initialize();
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) override;
	void UpdateBuffers(size_t currentImage, ImDrawData* pDrawData);

	~VulkanImGuiRenderer() final;
private:
	ImDrawData* m_pDrawData = nullptr;

	VkSampler m_SamplerRepeatLinear;
	VulkanTexture m_FontTexture;

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
	
	VkRenderPass myRenderPass;
};
#endif // !VULKAN_IMGUI_RENDERER_H
