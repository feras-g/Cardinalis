#ifndef VULKAN_PRESENT_RENDERER_H
#define VULKAN_PRESENT_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanPresentRenderer final : public VulkanRendererBase
{
public:
	VulkanPresentRenderer() = default;
	VulkanPresentRenderer(const VulkanContext& vkContext, bool useDepth);
	void render(size_t currentImageIdx, VkCommandBuffer cmdBuffer)  override;

	virtual bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts) override;
	virtual bool CreateRenderPass()   override;
	virtual bool CreateFramebuffers() override;

	~VulkanPresentRenderer() final;
private:
	bool bUseDepth;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H
