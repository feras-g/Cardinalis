#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720), bUseDepth(true) {}
	~SampleApp();
	void Initialize()	override;
	void Update()		override;
	void Render(size_t currentImageIdx)		override;
	void RenderGUI(size_t currentImageIdx)	override;
	void Terminate()	override;

	// List of renderers
	std::unique_ptr<VulkanClearColorRenderer> clear;
	std::unique_ptr<VulkanPresentRenderer> present;
	std::unique_ptr<VulkanImGuiRenderer> imgui;

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
	imgui = std::make_unique<VulkanImGuiRenderer>(context);
	imgui->Initialize();

}

void SampleApp::Update()
{

}

void SampleApp::Render(size_t currentImageIdx)
{
	const VulkanFrame& currentFrame  = m_RHI->GetCurrentFrame();
	const VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	//VK_CHECK(vkResetCommandBuffer(currentFrame.cmdBuffer, 0));
	VK_CHECK(vkResetCommandPool(context.device, currentFrame.cmdPool, 0));
	VK_CHECK(vkBeginCommandBuffer(currentFrame.cmdBuffer, &cmdBufferBeginInfo));
	
	clear->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	imgui->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	present->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::RenderGUI(size_t currentImageIdx)
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();

	ImGui::NewFrame();
	bool show_demo_window;
	ImGui::ShowDemoWindow(&show_demo_window);
	ImGui::Render();

	ImDrawData* draw_data = ImGui::GetDrawData();
	imgui->UpdateBuffers(currentImageIdx, draw_data);
}

inline void SampleApp::Terminate()
{
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

