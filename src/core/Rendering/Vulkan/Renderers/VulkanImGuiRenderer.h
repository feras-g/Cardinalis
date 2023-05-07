#pragma once

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
class ShadowRenderer;

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void init(const VulkanModelRenderer& model_renderer, const DeferredRenderer& deferred_renderer, const ShadowRenderer& shadow_renderer);
	void draw_scene(VkCommandBuffer cmd_buffer);
	void render(size_t currentImageIdx, VkCommandBuffer cmd_buffer);
	void create_buffers();
	void update_buffers(ImDrawData* pDrawData);

	float m_SceneViewAspectRatio = 1.0;
	
	void UpdateAttachments();

	std::array<int, NUM_FRAMES> m_ModelRendererColorTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererNormalTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererDepthTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererNormalMapTextureId;
	std::array<int, NUM_FRAMES> m_ModelRendererMetallicRoughnessTextureId;
	std::array<int, NUM_FRAMES> m_DeferredRendererOutputTextureId;
	std::array<int, NUM_FRAMES> m_ShadowRendererTextureId;

	~VulkanImGuiRenderer() ;
private:
	ImDrawData* m_pDrawData = nullptr;

	Texture2D m_FontTexture;
	std::vector<Texture2D> m_Textures;	// Textures displayed inside UI
	std::vector<VkDescriptorImageInfo> m_TextureDescriptors;

	// Storage buffers
	Buffer m_storage_buffer;
	Buffer m_uniform_buffer;

	std::unique_ptr<VulkanShader> m_Shader;

	bool CreateFontTexture(ImGuiIO* io, const char* fontPath, Texture2D& out_Font);
	bool CreatePipeline(VkDevice device);
	bool CreateUniformBuffers(size_t dataSizeInBytes);

	void update_descriptor_set(VkDevice device);

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
	VkPipeline m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorSet m_descriptor_set[NUM_FRAMES];
};
