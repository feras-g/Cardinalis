#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
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
	void UpdateGuiData(size_t currentImageIdx)	override;
	void UpdateRenderersData(size_t currentImageIdx) override;
	void Terminate()	override;

	// List of renderers
	enum ERenderer { CLEAR=0, PRESENT=1, IMGUI=2, MODEL=3, NUM_RENDERERS };
	inline VulkanRendererBase* GetRenderer(ERenderer name) { return renderers[name].get(); }

protected:
	bool bUseDepth;
};

void SampleApp::Initialize()
{
	renderers.push_back(std::make_unique<VulkanClearColorRenderer>(context, bUseDepth));
	renderers.push_back(std::make_unique<VulkanPresentRenderer>(context, bUseDepth));
	renderers.push_back(std::make_unique<VulkanImGuiRenderer>(context));
	renderers.push_back(std::make_unique<VulkanModelRenderer>("../../../data/models/cube.obj", "../../../data/textures/default.png"));
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
	GetRenderer(MODEL)->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	GetRenderer(PRESENT)->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::UpdateGuiData(size_t currentImageIdx)
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
		ImGui::Text("CPU : %.2f ms (%d fps)", Application::m_DeltaSeconds * 1000.0f, (int)m_FramePerfCounter->GetFps());
	}
	ImGui::End();

	ImGui::Render();
}

inline void SampleApp::UpdateRenderersData(size_t currentImageIdx)
{
	// Update ImGUI buffers
	UpdateGuiData(context.currentBackBuffer);
	// Other renderers data
	{
		glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), (float)m_Window->GetTimeSeconds(), glm::vec3(0, 1.0f, 0));
		glm::mat4 v = glm::lookAt(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		glm::mat4 p = glm::perspective(45.0f, m_Window->GetWidth() / (float)m_Window->GetHeight(), 0.1f, 1000.0f);
		((VulkanModelRenderer*)GetRenderer(MODEL))->UpdateBuffers(currentImageIdx, m, v, p);
	}
}

inline void SampleApp::Terminate()
{
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

