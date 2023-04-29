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
	RenderObjectManager::add_texture("../../../data/textures/albedo_default.png");
	RenderObjectManager::add_texture("../../../data/textures/placeholder.png");

	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/local/sponza-gltf-pbr/sponza.glb"), "sponza");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/cube.obj"), "cube");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/plane.glb"), "plane");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/suzanne.obj"), "suzanne");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/cow.obj"), "cow");
	RenderObjectManager::add_mesh(VulkanMesh("../../../data/models/bunny.obj"), "bunny");

	TransformData transform
	{
		glm::vec4(0,0,0,1),
		glm::vec4(1,1,1,1),
		glm::vec4(1,1,1, 0.0f)
	};
	RenderObjectManager::add_drawable(Drawable(RenderObjectManager::get_mesh("sponza"), true), "Sponza", transform);

	transform = 
	{
		glm::vec4(0,0,0,1),
		glm::vec4(1,1,1,0),
		glm::vec4(100,1,100, 1.0f)
	};
	RenderObjectManager::add_drawable(Drawable(RenderObjectManager::get_mesh("plane")), "Plane", transform);


	RenderObjectManager::configure();
	m_model_renderer.reset(new VulkanModelRenderer);
	m_imgui_renderer.reset(new VulkanImGuiRenderer(context));
	
	m_deferred_renderer.init(
		m_model_renderer->m_Albedo_Output, 
		m_model_renderer->m_Normal_Output, 
		m_model_renderer->m_Depth_Output,
		m_model_renderer->m_NormalMap_Output,
		m_model_renderer->m_MetallicRoughness_Output
	);

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

	UpdateRenderersData(dt, context.curr_frame_idx);
}

void SampleApp::Render()
{
	VulkanFrame current_frame = context.frames[context.curr_frame_idx];

	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VK_CHECK(vkWaitForFences(context.device, 1, &current_frame.fence_queue_submitted, true, OneSecondInNanoSeconds));
	VK_CHECK(vkResetFences(context.device, 1, &current_frame.fence_queue_submitted));

	uint32_t swapchain_buffer_idx = 0;
	VkResult result = swapchain.AcquireNextImage(current_frame.smp_image_acquired, &swapchain_buffer_idx);

	// Populate command buffers
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VK_CHECK(vkResetCommandPool(context.device, current_frame.cmd_pool, 0));
		VK_CHECK(vkBeginCommandBuffer(current_frame.cmd_buffer, &cmdBufferBeginInfo));

		/* Transition to color attachment */
		swapchain.color_attachments[context.curr_frame_idx].transition_layout(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		{
			m_model_renderer->render(context.curr_frame_idx, current_frame.cmd_buffer);
			m_deferred_renderer.render(context.curr_frame_idx, current_frame.cmd_buffer);
			m_imgui_renderer->render(context.curr_frame_idx, current_frame.cmd_buffer);
		}

		/* Present */
		swapchain.color_attachments[context.curr_frame_idx].transition_layout(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(current_frame.cmd_buffer));
	}

	// Submit commands for the GPU to work on the current backbuffer
	// Has to wait for the swapchain image to be acquired before beginning, we wait on imageAcquired semaphore.
	// Signals a renderComplete semaphore to let the next operation know that it finished


	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &current_frame.smp_image_acquired,
		.pWaitDstStageMask = &waitDstStageMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &current_frame.cmd_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &current_frame.smp_queue_submitted
	};
	vkQueueSubmit(context.queue, 1, &submit_info, current_frame.fence_queue_submitted);

	// Present work
	// Waits for the GPU queue to finish execution before presenting, we wait on renderComplete semaphore

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &current_frame.smp_queue_submitted,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &swapchain_buffer_idx
	};

	vkQueuePresentKHR(context.queue, &presentInfo);

	context.frame_count++;
	context.curr_frame_idx = (context.curr_frame_idx + 1) % NUM_FRAMES;
}

inline void SampleApp::UpdateRenderersData(float dt, size_t currentImageIdx)
{
	if (m_UI.fSceneViewAspectRatio != m_Camera.aspect_ratio)
	{
		m_Camera.UpdateAspectRatio(m_UI.fSceneViewAspectRatio);
	}
	m_Camera.controller.deltaTime = dt;
	m_Camera.controller.UpdateTranslation(dt);

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
		VulkanRendererBase::update_frame_data(frame_data, currentImageIdx);
	}

	/* Update drawables */
	RenderObjectManager::update_per_object_data(frame_data);

	m_deferred_renderer.update(currentImageIdx);

	// ImGui composition
	{
		m_UI.Start();
		//m_UI.AddHierarchyPanel();
		//m_UI.ShowMenuBar();
		//m_UI.AddInspectorPanel();
		m_UI.ShowStatistics(m_DebugName, dt, context.frame_count);
		m_UI.ShowSceneViewportPanel(
			m_imgui_renderer->m_DeferredRendererOutputTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererColorTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererNormalTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererDepthTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererNormalMapTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererMetallicRoughnessTextureId[currentImageIdx]
		);
		//m_UI.ShowFrameTimeGraph(FrameStats::History.data(), FrameStats::History.size());
		m_UI.ShowCameraSettings(&m_Camera);
		m_UI.ShowInspector();
		m_UI.ShowLightSettings(&m_deferred_renderer.m_light_manager);
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
	RenderObjectManager::destroy();
	VulkanRendererBase::destroy();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

