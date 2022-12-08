#ifndef VULKAN_PRESENT_RENDERER_H
#define VULKAN_PRESENT_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanPresentRenderer final : public VulkanRendererBase
{
public:
	VulkanPresentRenderer() = default;
	VulkanPresentRenderer(const VulkanContext& vkContext, bool useDepth);
	void Initialize();
	void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) const override;


	~VulkanPresentRenderer() final;
private:
	bool bUseDepth;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H
