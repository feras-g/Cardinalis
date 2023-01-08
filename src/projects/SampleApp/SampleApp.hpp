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

protected:
	bool bUseDepth;
};

void SampleApp::Initialize()
{
	m_ClearRenderer.reset(new VulkanClearColorRenderer(context, bUseDepth));
	m_PresentRenderer.reset(new VulkanPresentRenderer(context, bUseDepth));
	m_ModelRenderer.reset(new VulkanModelRenderer ("../../../data/models/suzanne.obj", "../../../data/textures/default.png"));
	m_ImGuiRenderer.reset(new VulkanImGuiRenderer(context));
	m_ImGuiRenderer->Initialize(m_ModelRenderer->m_ColorAttachments);
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

	m_ClearRenderer->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	m_ModelRenderer->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	m_ImGuiRenderer->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);
	m_PresentRenderer->PopulateCommandBuffer(currentImageIdx, currentFrame.cmdBuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
}

inline void SampleApp::UpdateGuiData(size_t currentImageIdx)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::NewFrame();
	ImGui::StyleColorsDark();

	// Viewport
	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

	// Scene view
	if (ImGui::Begin("Scene", 0, 0))
	{
		ImVec2 sceneViewPanelSize = ImGui::GetContentRegionAvail();

		m_ImGuiRenderer->m_SceneViewAspectRatio = sceneViewPanelSize.x / sceneViewPanelSize.y;
		ImGui::Image((ImTextureID)m_ImGuiRenderer->m_ModelRendererColorTextureId[currentImageIdx], sceneViewPanelSize);
	}
	ImGui::End();
	
	// Hierachy panel
	if (ImGui::Begin("Hierarchy", 0, 0))
	{

	}
	ImGui::End();

	// Toolbar
	// TODO

	// Inspector panel
	if (ImGui::Begin("Inspector", 0, 0))
	{

	}
	ImGui::End();


	// Overlay
	ImGuiWindowFlags overlayFlags = 
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | 
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;
	
	ImVec2 work_pos  = viewport->WorkPos;
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_pos.y + work_size.y - PAD);

	window_pos_pivot.x = 0.0f;
	window_pos_pivot.y = 1.0f;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(0.33f); // Transparent background
	if (ImGui::Begin("Overlay", 0, overlayFlags))
	{
		ImGui::Text(m_DebugName);
		ImGui::AlignTextToFramePadding();
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
		static float angle = 0.0F;
		angle += 0.00021 / Application::m_DeltaSeconds;
		glm::mat4 m = 
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * 
			glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 v = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 p = glm::perspective(45.0f, m_ImGuiRenderer->m_SceneViewAspectRatio, 0.1f, 1000.0f);

		m_ModelRenderer->UpdateBuffers(currentImageIdx, m, v, p);
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

