#pragma once

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/RenderPass.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/ShadowRenderer.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

struct ImDrawData;
struct ImGuiIO;
class VulkanModelRenderer;
struct DeferredRenderer;
struct ShadowRenderer;

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	VulkanImGuiRenderer(const VulkanContext& vkContext);
	void init();
	void render(size_t currentImageIdx, VkCommandBuffer cmd_buffer);
	void update_render_pass_attachments();

	std::array<VkDescriptorSet, NUM_FRAMES> m_ModelRendererColorTextureId;
	std::array<VkDescriptorSet, NUM_FRAMES> m_ModelRendererNormalTextureId;
	std::array<VkDescriptorSet, NUM_FRAMES> m_ModelRendererDepthTextureId;
	std::array<VkDescriptorSet, NUM_FRAMES> m_ModelRendererNormalMapTextureId;
	std::array<VkDescriptorSet, NUM_FRAMES> m_ModelRendererMetallicRoughnessTextureId;
	std::array<VkDescriptorSet, NUM_FRAMES> m_DeferredRendererOutputTextureId;

	std::array<std::array<VkDescriptorSet, 4>, NUM_FRAMES> m_CascadedShadowRenderer_Textures;

	void destroy();
private:
	ImDrawData* m_pDrawData = nullptr;

	Texture2D m_FontTexture;
	std::vector<VkImageView*> m_Textures;	// Textures displayed inside UI
	std::vector<VkDescriptorImageInfo> m_TextureDescriptors;

	VertexFragmentShader m_Shader;


	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
	VkDescriptorPool m_descriptor_pool;
};
