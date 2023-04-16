#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
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
	void Render() override;
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
	std::unique_ptr<VulkanImGuiRenderer>		m_imgui_renderer;
	std::unique_ptr<VulkanModelRenderer>		m_model_renderer;
	DeferredRenderer							m_deferred_renderer;
};

void SampleApp::Initialize()
{
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/plane.obj"), "Plane");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/suzanne.obj"), "Suzanne");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/cow.obj"), "Cow");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/bunny.obj"), "bunny");

	int idx = 0;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			for (int k = 0; k < 5; k++)
			{

				glm::mat4 random_rot = glm::identity<glm::mat4>();
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-1.f, 1.f);

					glm::vec3 axis(dis(gen), dis(gen), dis(gen));
					axis = glm::normalize(axis);
					float angle = glm::pi<float>() * dis(gen);

					random_rot = glm::rotate(random_rot, angle, axis);
				}



				glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(i, j, k)) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.2, 0.2, 0.2)) * random_rot;

				std::string name = "Drawable" + std::to_string(idx++);
				if ( ((i + j + k) % 2) == 0)
				{
					RenderObjectManager::add_drawable(Drawable(RenderObjectManager::get_mesh("Suzanne"), model), name.c_str());
				}
				else
				{
					RenderObjectManager::add_drawable(Drawable(RenderObjectManager::get_mesh("bunny"), model), name.c_str());
				}
			}
		}

	}

	RenderObjectManager::configure();
	m_model_renderer.reset(new VulkanModelRenderer);
	m_imgui_renderer.reset(new VulkanImGuiRenderer(context));
	
	m_deferred_renderer.init(
		m_model_renderer->m_Albedo_Output, 
		m_model_renderer->m_Normal_Output, 
		m_model_renderer->m_Depth_Output);

	m_imgui_renderer->Initialize(*m_model_renderer.get(), m_deferred_renderer);

	CameraController fpsController = CameraController({ 0,0,5 }, { 0,-180,0 }, { 0,0,1 }, { 0,1,0 });

	m_Camera = Camera(fpsController, 45.0f, m_imgui_renderer->m_SceneViewAspectRatio, 0.1f, 1000.0f);
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
	if (EngineGetAsyncKeyState(Key::LSHIFT)) m_Camera.controller.m_movement =  m_Camera.controller.m_movement | Movement::DOWN;

	if (m_UI.fSceneViewAspectRatio != m_Camera.aspect_ratio)
	{
		m_Camera.UpdateAspectRatio(m_UI.fSceneViewAspectRatio);
	}
	m_Camera.controller.deltaTime = dt;
	m_Camera.controller.UpdateTranslation(dt);

	UpdateRenderersData(dt, context.currentBackBuffer);
}

void SampleApp::Render()
{
	static uint32_t current_frame_idx = 0;
	VulkanFrame currentFrame = context.frames[current_frame_idx];

	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VK_CHECK(vkWaitForFences(context.device, 1, &currentFrame.renderFence, true, OneSecondInNanoSeconds));
	VK_CHECK(vkResetFences(context.device, 1, &currentFrame.renderFence));

	VkResult result = swapchain.AcquireNextImage(currentFrame.imageAcquiredSemaphore, &context.currentBackBuffer);

	// Populate command buffers
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VK_CHECK(vkResetCommandPool(context.device, currentFrame.cmd_pool, 0));
		VK_CHECK(vkBeginCommandBuffer(currentFrame.cmd_buffer, &cmdBufferBeginInfo));

		/* Transition to color attachment */
		swapchain.color_attachments[current_frame_idx].transition_layout(currentFrame.cmd_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		{
			m_model_renderer->render(current_frame_idx, currentFrame.cmd_buffer);
			m_deferred_renderer.render(current_frame_idx, currentFrame.cmd_buffer);
			m_imgui_renderer->render(current_frame_idx, currentFrame.cmd_buffer);
		}

		/* Present */
		swapchain.color_attachments[current_frame_idx].transition_layout(currentFrame.cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(currentFrame.cmd_buffer));
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
		.pCommandBuffers = &currentFrame.cmd_buffer,
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
	current_frame_idx = (current_frame_idx + 1) % NUM_FRAMES;
}

inline void SampleApp::UpdateRenderersData(float dt, size_t currentImageIdx)
{
	//vkWaitForFences(context.device, 1, &context.frames[currentImageIdx].renderFence, TRUE, UINT64_MAX);
	
	/* Update frame Data */
	VulkanRendererBase::PerFrameData frame_data;
	{
		frame_data.view = m_Camera.GetView();
		frame_data.proj = m_Camera.GetProj();
		frame_data.inv_view_proj = glm::inverse(frame_data.proj * frame_data.view);
		frame_data.view_pos = glm::vec4(
			m_Camera.controller.m_position.x,
			m_Camera.controller.m_position.y,
			m_Camera.controller.m_position.z,
			1.0);
		VulkanRendererBase::update_frame_data(frame_data);
	}

	/* Update drawables */
	RenderObjectManager::update_drawables(frame_data);

	m_deferred_renderer.update(currentImageIdx);

	// ImGui composition
	{
		m_UI.Start();
		//m_UI.AddHierarchyPanel();
		//m_UI.ShowMenuBar();
		//m_UI.AddInspectorPanel();
		m_UI.ShowStatistics(m_DebugName, dt, context.frameCount);
		m_UI.ShowSceneViewportPanel(
			m_imgui_renderer->m_DeferredRendererOutputTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererColorTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererNormalTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererDepthTextureId[currentImageIdx]);
		//m_UI.ShowFrameTimeGraph(FrameStats::History.data(), FrameStats::History.size());
		//m_UI.ShowCameraSettings(&m_Camera);
		m_UI.End();
	}

	{
		m_imgui_renderer->update_buffers(ImGui::GetDrawData());
	}
}

inline void SampleApp::OnWindowResize()
{
	context.swapchain->Reinitialize();
	m_imgui_renderer->UpdateAttachments();
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

