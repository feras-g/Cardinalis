#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"

class SampleApp : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720) {}
	~SampleApp();
	void Initialize()	override final;
	void Update()		override final;
	void Render()		override final;
	void Terminate()	override final;
protected:
	VkRenderPass renderPass;
};

void SampleApp::Initialize()
{
	renderPass = m_RHI->CreateExampleRenderPass();
	m_RHI->CreateFramebuffers(renderPass);

	VulkanShader exampleShader("ExampleShader.vert.spv", "ExampleShader.frag.spv");
}

void SampleApp::Update()
{

}

void SampleApp::Render()
{
	const VulkanFrame& currentFrame  = m_RHI->GetCurrentFrame();
	const VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();
	VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VK_CHECK(vkResetCommandBuffer(currentFrame.cmdBuffer, 0));
	VK_CHECK(vkBeginCommandBuffer(currentFrame.cmdBuffer, &cmdBufferBeginInfo));

	VkClearValue clearColor = { .color = { 0.1f, 0.0f, 0.1f, 1.0f } };	// Clear renderPass attachments
	VkRenderPassBeginInfo beginRenderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = swapchain.framebuffers[context.currentBackBuffer],
		.renderArea {.extent = swapchain.metadata.extent },
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// APPLICATION SPECIFIC RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vkCmdBeginRenderPass(currentFrame.cmdBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdEndRenderPass(currentFrame.cmdBuffer);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::Terminate()
{
}

inline SampleApp::~SampleApp()
{
	vkDestroyRenderPass(context.device, renderPass, nullptr);
}

#endif // !SAMPLE_APP_H

