#include "PostProcessingRenderer.h"
#include "Rendering/Vulkan/VulkanShader.h"

static VulkanShader s_shaders[PostProcessRenderer::FX::eCount] =
{
	VulkanShader("Downsample.compute.spv"),
	VulkanShader(),
	VulkanShader(),
};

void PostProcessRenderer::init()
{

}

void PostProcessRenderer::render(FX fx)
{

}

void PostProcessRenderer::render_downsample()
{

}