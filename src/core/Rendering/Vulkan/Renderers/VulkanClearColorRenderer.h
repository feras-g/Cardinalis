#ifndef VULKAN_CLEAR_COLOR_RENDERER_H
#define VULKAN_CLEAR_COLOR_RENDERER_H

#include "Rendering/Vulkan/VulkanRendererBase.h"

class VulkanClearColorRenderer : public VulkanRendererBase
{
public:
	VulkanClearColorRenderer(const VulkanContext& vkContext);
	void Initialize();


	~VulkanClearColorRenderer() final;
private:
	bool bClearDepth;
};
#endif // !VULKAN_CLEAR_COLOR_RENDERER_H
