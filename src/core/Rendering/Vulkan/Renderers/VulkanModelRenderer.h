#ifndef VULKAN_MODEL_RENDERER_H
#define VULKAN_MODEL_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanModel.h"
#include "Rendering/Vulkan/RenderPass.h"

class VulkanShader;

class VulkanModelRenderer final : public VulkanRendererBase
{
	friend class VulkanImGuiRenderer;
public:
	VulkanModelRenderer() = default;
	explicit VulkanModelRenderer(const char* modelFilename);
	void render(size_t currentImageIdx, VkCommandBuffer cmdBuffer)  override;

	bool Init();
	void draw_scene(size_t currentImageIdx, VkCommandBuffer cmdBuffer);
	bool UpdateBuffers(size_t currentImage, glm::mat4 model, glm::mat4 view, glm::mat4 proj);
	bool UpdateDescriptorSets(VkDevice device) final override;
	void create_attachments();

	// Offscreen images
	std::array<Texture2D, NUM_FRAMES> m_Albedo_Output;
	std::array<Texture2D, NUM_FRAMES> m_Normal_Output;
	std::array<Texture2D, NUM_FRAMES> m_Depth_Output;

	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];

	/** Size in pixels of the offscreen buffers */
	static const uint32_t render_width = 1024;
	static const uint32_t render_height = 1024;

	~VulkanModelRenderer() final;
private:
	VulkanModel m_Model;
	std::unique_ptr<VulkanShader> m_Shader;

	std::vector<VkFormat> m_ColorAttachmentFormats = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R16G16B16A16_SFLOAT };
	VkFormat m_DepthAttachmentFormat	= VK_FORMAT_D16_UNORM;
	VkPipeline			m_GraphicsPipeline = VK_NULL_HANDLE;

	VkSampler m_TextureSampler;
	Texture2D m_Texture;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H