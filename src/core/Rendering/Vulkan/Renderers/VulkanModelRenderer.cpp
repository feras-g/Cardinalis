#include "VulkanModelRenderer.h"

VulkanModelRenderer::VulkanModelRenderer(const char* modelFilename, const char* textureFilename, uint32_t uniformDataSizeInBytes) 
	: VulkanRendererBase(context, true)
{
}

void VulkanModelRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) const
{
}

bool VulkanModelRenderer::CreateRenderPass()
{
	return false;
}

bool VulkanModelRenderer::CreateFramebuffers()
{
	return false;
}

VulkanModelRenderer::~VulkanModelRenderer()
{
}
