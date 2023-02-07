#ifndef VULKAN_CLEAR_COLOR_RENDERER_H
#define VULKAN_CLEAR_COLOR_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanClearColorRenderer final : public VulkanRendererBase
{
public:
	VulkanClearColorRenderer() = delete;
	VulkanClearColorRenderer(const VulkanContext& vkContext);
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) override;

	virtual bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts) override;

	bool CreateFramebufferAndRenderPass();

	virtual bool CreateRenderPass()   override;
	virtual bool CreateFramebuffers() override;

	~VulkanClearColorRenderer() final;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H