#ifndef VULKAN_CLEAR_COLOR_RENDERER_H
#define VULKAN_CLEAR_COLOR_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanClearColorRenderer final : public VulkanRendererBase
{
public:
	VulkanClearColorRenderer() = default;
	VulkanClearColorRenderer(const VulkanContext& vkContext, bool useDepth);
	void Initialize();
	void PopulateCommandBuffer(VkCommandBuffer cmdBuffer) const override;


	~VulkanClearColorRenderer() final;
private:
	bool bClearDepth;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H
