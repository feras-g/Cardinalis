#ifndef VULKAN_IMGUI_RENDERER_H
#define VULKAN_IMGUI_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

struct ImDrawData;
struct ImGuiIO;

class VulkanImGuiRenderer final : public VulkanRendererBase
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void Initialize();
	void PopulateCommandBuffer(VkCommandBuffer cmdBuffer) const override;

	~VulkanImGuiRenderer() final;
private:
	ImDrawData* drawData = nullptr;

	VkSampler m_FontSampler;
	VulkanTexture m_FontTexture;

	// Storage buffers
	VkBuffer m_StorageBuffers[NUM_FRAMES];
	VkDeviceMemory m_StorageBufferMems[NUM_FRAMES];

	bool CreateFontTexture(ImGuiIO* io, const char* fontPath, VulkanTexture& out_Font);
	bool CreateTextureSampler(VkDevice device, VkFilter minFilter, VkFilter maxFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler);
	bool CreatePipeline(VkDevice device);
	bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts);
	bool CreatePipelineLayout(VkDevice device, VkDescriptorSetLayout descSetLayout, VkPipelineLayout* out_PipelineLayout);
};
#endif // !VULKAN_IMGUI_RENDERER_H
