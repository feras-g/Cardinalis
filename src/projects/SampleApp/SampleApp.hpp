#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/FrameCounter.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720), bUseDepth(true) 
	{

	}
	~SampleApp();
	void Initialize()	override;
	void Update()		override;
	void Render(size_t currentImageIdx)		override;
	void RenderGUI(size_t currentImageIdx)	override;
	void Terminate()	override;

	// List of renderers
	enum ERenderer { CLEAR=0, PRESENT=1, IMGUI=2, NUM_RENDERERS };
	inline VulkanRendererBase* GetRenderer(ERenderer name) { return renderers[name].get(); }

protected:
	bool bUseDepth;
};

void SampleApp::Initialize()
{
	renderers.push_back(std::make_unique<VulkanClearColorRenderer>(context, bUseDepth));
	renderers.push_back(std::make_unique<VulkanPresentRenderer>(context, bUseDepth));
	renderers.push_back(std::make_unique<VulkanImGuiRenderer>(context));
}

void SampleApp::Update()
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();
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
	
	GetRenderer(CLEAR)->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	GetRenderer(IMGUI)->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	GetRenderer(PRESENT)->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::RenderGUI(size_t currentImageIdx)
{
	ImGui::NewFrame();
	ImGui::StyleColorsDark();

	// Viewport
	ImGui::DockSpaceOverViewport();

	// Scene view
	if (ImGui::Begin("Scene", 0, 0))
	{
		ImGui::BeginTabBar("Scene");
		ImGui::EndTabBar();
	}
	ImGui::End();

	// Overlay
	ImGuiWindowFlags overlayFlags = 
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | 
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;
	
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_pos.y + PAD);

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.33f); // Transparent background
	if (ImGui::Begin("Overlay", 0, overlayFlags))
	{
		ImGui::Text(m_DebugName);
		ImGui::Text("CPU : %.1f ms (%d fps)", Application::m_DeltaSeconds * 1000.0f, (int)m_FramePerfCounter->GetFps());
	}
	ImGui::End();

	ImGui::Render();

	ImDrawData* draw_data = ImGui::GetDrawData();
	((VulkanImGuiRenderer*)GetRenderer(IMGUI))->UpdateBuffers(currentImageIdx, draw_data);
}

inline void SampleApp::Terminate()
{
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

