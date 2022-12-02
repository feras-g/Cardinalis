#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720), bUseDepth(true) {}
	~SampleApp();
	void Initialize()	override;
	void Update()		override;
	void Render()		override;
	void Terminate()	override;

	// List of renderers
	std::unique_ptr<VulkanClearColorRenderer> clear;
	std::unique_ptr<VulkanPresentRenderer> present;

protected:
	bool bUseDepth;
};

void SampleApp::Initialize()
{
	// Shaders
	VulkanShader exampleShader("ExampleShader.vert.spv", "ExampleShader.frag.spv");

	// Renderers
	clear   = std::make_unique<VulkanClearColorRenderer>(context, bUseDepth);
	clear->Initialize();
	present = std::make_unique<VulkanPresentRenderer>(context, bUseDepth);
	present->Initialize();
}

void SampleApp::Update()
{

}

void SampleApp::Render()
{
	const VulkanFrame& currentFrame  = m_RHI->GetCurrentFrame();
	const VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkResetCommandBuffer(currentFrame.cmdBuffer, 0));
	VK_CHECK(vkBeginCommandBuffer(currentFrame.cmdBuffer, &cmdBufferBeginInfo));
	
	clear->PopulateCommandBuffer(currentFrame.cmdBuffer);
	present->PopulateCommandBuffer(currentFrame.cmdBuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::Terminate()
{
	
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

