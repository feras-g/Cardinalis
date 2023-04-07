#ifndef VULKAN_IMGUI_RENDERER_H
#define VULKAN_IMGUI_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/RenderPass.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

struct ImDrawData;
struct ImGuiIO;
class VulkanShader;
class VulkanModelRenderer;
class DeferredRenderer;

class VulkanImGuiRenderer : public VulkanRendererBase
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void Initialize(const VulkanModelRenderer& model_renderer, const DeferredRenderer& deferred_renderer);
	void draw_scene(VkCommandBuffer cmd_buffer);
	void render(size_t currentImageIdx, VkCommandBuffer cmd_buffer) override;
	void update_buffers(size_t currentImage, ImDrawData* pDrawData);
	bool RecreateFramebuffersRenderPass();

	float m_SceneViewAspectRatio = 1.0;
	bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts) override;
	void UpdateAttachments();

	std::array<int, NUM_FRAMES> m_ModelRendererColorTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererNormalTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererDepthTextureId;
	std::array<int, NUM_FRAMES> m_DeferredRendererOutputTextureId;

	~VulkanImGuiRenderer() override final;
private:
	ImDrawData* m_pDrawData = nullptr;

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];

	VkSampler m_SamplerRepeatLinear;
	Texture2D m_FontTexture;
	std::vector<Texture2D> m_Textures;	// Textures displayed inside UI
	std::vector<VkDescriptorImageInfo> m_TextureDescriptors;



	// Storage buffers
	Buffer m_StorageBuffers[NUM_FRAMES];

	std::unique_ptr<VulkanShader> m_Shader;

	bool CreateFontTexture(ImGuiIO* io, const char* fontPath, Texture2D& out_Font);
	bool CreatePipeline(VkDevice device);
	bool CreateUniformBuffers(size_t dataSizeInBytes);

	bool UpdateDescriptorSets(VkDevice device) final override;

	VkDescriptorPool m_ImGuiDescriptorPool;

};
#endif // !VULKAN_IMGUI_RENDERER_H
