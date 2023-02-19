#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/Vulkan/VulkanUI.h"
#include "Rendering/Camera.h"
#include "Rendering/FrameCounter.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720)
	{

	}
	~SampleApp();
	void Initialize()	override;
	void Update(float dt)		override;
	void Render(size_t currentImageIdx)		override;
	void UpdateRenderersData(float dt, size_t currentImageIdx) override;
	void Terminate()	override;

	void OnWindowResize() override;
	void OnLeftMouseButtonUp() override;
	void OnLeftMouseButtonDown() override;
	void OnMouseMove(int x, int y) override;
	void OnKeyEvent(KeyEvent event);

protected:
	VulkanUI m_UI;
	Camera m_Camera;

	bool b_IsSceneViewportHovered = false;

protected:
	std::unique_ptr<VulkanImGuiRenderer>		m_ImGuiRenderer;
	std::unique_ptr<VulkanModelRenderer>		m_ModelRenderer;
	std::unique_ptr<VulkanPresentRenderer>		m_PresentRenderer;
	std::unique_ptr<VulkanClearColorRenderer>	m_ClearColorRenderer;
};

void SampleApp::Initialize()
{
	m_PresentRenderer.reset(new VulkanPresentRenderer(context, false));
	m_ModelRenderer.reset(new VulkanModelRenderer ("../../../data/models/suzanne.obj", "../../../data/textures/default.png"));
	m_ImGuiRenderer.reset(new VulkanImGuiRenderer(context));
	m_ImGuiRenderer->Initialize(m_ModelRenderer->m_ColorAttachments);
	m_ClearColorRenderer.reset(new VulkanClearColorRenderer(context));

	CameraController fpsController = CameraController({ 0, 0, -5 }, { 0,0,1 }, { 0, 1, 0 });

	m_Camera = Camera(fpsController, 45.0f, m_ImGuiRenderer->m_SceneViewAspectRatio, 0.1f, 1000.0f);
}

void SampleApp::Update(float dt)
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();

	if (EngineGetAsyncKeyState(Key::Z))     m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::FORWARD;
	if (EngineGetAsyncKeyState(Key::Q))     m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::LEFT;
	if (EngineGetAsyncKeyState(Key::S))     m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::BACKWARD;
	if (EngineGetAsyncKeyState(Key::D))     m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::RIGHT;
	if (EngineGetAsyncKeyState(Key::SPACE)) m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::UP;

	//if (m_CameraController.m_movement != Movement::NONE)
	{
		if (m_UI.fSceneViewAspectRatio != m_Camera.aspect_ratio)
		{
			m_Camera.UpdateAspectRatio(m_UI.fSceneViewAspectRatio);
		}
		m_Camera.controller.deltaTime = dt;
		m_Camera.controller.UpdateTranslation(dt);
	}

	UpdateRenderersData(dt, context.currentBackBuffer);
}

void SampleApp::Render(size_t currentImageIdx)
{
	VulkanFrame& currentFrame = context.frames[context.frameCount % NUM_FRAMES];
	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*VK_CHECK*/(vkWaitForFences(context.device, 1, &currentFrame.renderFence, true, OneSecondInNanoSeconds));
	/*VK_CHECK*/(vkResetFences(context.device, 1, &currentFrame.renderFence));

	VkResult result = swapchain.AcquireNextImage(currentFrame.imageAcquiredSemaphore, &context.currentBackBuffer);

	// Populate command buffers
	{	
		VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VK_CHECK(vkResetCommandPool(context.device, currentFrame.cmdPool, 0));
		VK_CHECK(vkBeginCommandBuffer(currentFrame.cmdBuffer, &cmdBufferBeginInfo));

		m_ClearColorRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_ModelRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_ImGuiRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_PresentRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);

		VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
	}

	// Submit commands for the GPU to work on the current backbuffer
	// Has to wait for the swapchain image to be acquired before beginning, we wait on imageAcquired semaphore.
	// Signals a renderComplete semaphore to let the next operation know that it finished

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentFrame.imageAcquiredSemaphore,
		.pWaitDstStageMask = &waitDstStageMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &currentFrame.cmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentFrame.queueSubmittedSemaphore
	};
	vkQueueSubmit(context.queue, 1, &submitInfo, currentFrame.renderFence);

	// Present work
	// Waits for the GPU queue to finish execution before presenting, we wait on renderComplete semaphore
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentFrame.queueSubmittedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &context.currentBackBuffer
	};

	vkQueuePresentKHR(context.queue, &presentInfo);

	context.frameCount++;
}

inline void SampleApp::UpdateRenderersData(float dt, size_t currentImageIdx)
{
	// ImGui composition
	{
		m_UI.Start();
		m_UI.AddHierarchyPanel();
		m_UI.ShowMenuBar();
		m_UI.AddInspectorPanel();
		m_UI.AddOverlay(m_DebugName, dt);
		m_UI.ShowSceneViewportPanel(
			m_ImGuiRenderer->m_ModelRendererColorTextureId[currentImageIdx],
			m_ImGuiRenderer->m_ModelRendererDepthTextureId[currentImageIdx]);
		m_UI.ShowPlot(lastTimeSteps, 500);
		m_UI.End();
	}

	{
		glm::mat4 m = glm::identity<glm::mat4>();
		glm::mat4 v = m_Camera.GetView();
		glm::mat4 p = m_Camera.GetProj();
		
		m_ModelRenderer->UpdateBuffers(currentImageIdx, m, v, p);
	}
}

inline void SampleApp::OnWindowResize()
{
	context.swapchain->Reinitialize();
	m_ClearColorRenderer->CreateFramebufferAndRenderPass();
	m_PresentRenderer->RecreateFramebuffersRenderPass();
	m_ImGuiRenderer->RecreateFramebuffersRenderPass();
}

inline void SampleApp::OnLeftMouseButtonUp()
{
	Application::OnLeftMouseButtonUp();
}

void SampleApp::OnLeftMouseButtonDown()
{
	Application::OnLeftMouseButtonDown();
	m_Camera.controller.m_last_mouse_pos = { m_MouseEvent.px, m_MouseEvent.py };
}

inline void SampleApp::OnMouseMove(int x, int y)
{
	Application::OnMouseMove(x, y);
	if (m_UI.bIsSceneViewportHovered)
	{
		// Camera rotation
		if (m_MouseEvent.bLeftClick)
		{
			if (m_MouseEvent.bFirstClick)
			{
				m_MouseEvent.lastClickPosX = x;
				m_MouseEvent.lastClickPosY = y;

				m_MouseEvent.bFirstClick = false;
			}
			else
			{
				m_Camera.controller.UpdateRotation({x, y});

				m_MouseEvent.lastClickPosX = x;
				m_MouseEvent.lastClickPosY = y;
			}
		}
	}
}

inline void SampleApp::OnKeyEvent(KeyEvent event)
{

}

inline void SampleApp::Terminate()
{
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

