#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"

#include "Rendering/Vulkan/VulkanUI.h"
#include "Rendering/Camera.h"
#include "Rendering/FrameCounter.h"
#include "Rendering/LightManager.h"

#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/ShadowRenderer.h"
#include "Rendering/Vulkan/Renderers/CubemapRenderer.h"
#include "Rendering/Vulkan/Renderers/PostProcessingRenderer.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720)
	{

	}
	~SampleApp();
	void Initialize() override;
	void InitSceneResources();
	void LoadMesh(std::string_view path, std::string_view name);
	void AddDrawable(std::string_view mesh_name, bool visible = true, 
	                 glm::vec3 position = {0,0,0}, glm::vec3 rotation = {0,0,0}, glm::vec3 scale = {1,1,1});
	void Update(float dt) override;
	void Render() override;
	void UpdateRenderersData(float dt, size_t currentImageIdx) override;
	void Terminate() override;

	void OnWindowResize() override;
	void OnLeftMouseButtonUp() override;
	void OnLeftMouseButtonDown() override;
	void OnMouseMove(int x, int y) override;
	void OnKeyEvent(KeyEvent event);

protected:
	VulkanUI m_UI;
	Camera m_Camera;
	LightManager m_light_manager;
	RenderObjectManager m_rbo;
	bool b_IsSceneViewportHovered = false;

protected:
	std::unique_ptr<VulkanImGuiRenderer>		m_imgui_renderer;
	std::unique_ptr<VulkanModelRenderer>		m_model_renderer;
	DeferredRenderer m_deferred_renderer;
	ShadowRenderer m_shadow_renderer;
	CascadedShadowRenderer m_cascaded_shadow_renderer;
	CubemapRenderer m_cubemap_renderer;
	PostProcessRenderer m_postprocess;
};

void SampleApp::InitSceneResources()
{
	CameraController fpsController = CameraController( { 0, 0, 5 }, { 0, -180, 0 }, { 0, 0, 1 }, { 0, 1, 0 });
	m_Camera = Camera(fpsController, 45.0f, 1.0f, 0.25f, 250.0f);

	m_rbo.init();

	/* Basic primitives */
	{
		VulkanMesh mesh = VulkanMesh("../../../data/models/basic/unit_cube.glb");
		uint32_t id = m_rbo.add_mesh(mesh, "cube");
		m_rbo.add_drawable(id, "skybox", false);
		m_rbo.add_drawable(id, "placeholder_cube", false);
	}

	LoadMesh("../../../data/models/basic/unit_plane.glb", "Floor");
	LoadMesh("../../../data/models/test/metalroughspheres/MetalRoughSpheres.gltf", "MetalRoughSpheres");
	LoadMesh("../../../data/models/basic/duck/duck.gltf", "ColladaDuck");
	//LoadMesh("../../../data/models/scenes/sponza-gltf-pbr/sponza.glb", "Sponza");

	glm::ivec3 dimensions(5,5,5);
	float spacing = 2.5f;
	for (int x = 0; x < dimensions.x; x++)
	{
		for (int y = 0; y < dimensions.y; y++)
		{
			for (int z = 0; z < dimensions.z; z++)
			{
				glm::vec3 position{ x, y, z };
				AddDrawable("ColladaDuck", true, position * spacing, { 0, 0, 0 }, { 0.1, 0.1, 0.1 });
			}
		}
	}

	m_rbo.configure();
}

inline void SampleApp::LoadMesh(std::string_view path, std::string_view name)
{
	VulkanMesh mesh = VulkanMesh(path.data());
	uint32_t id = m_rbo.add_mesh(mesh, name.data());
	std::string drawable_name("Drawable" + std::to_string(m_rbo.drawables.size()) + "_" + name.data());
	m_rbo.add_drawable(id, drawable_name);
}


inline void SampleApp::AddDrawable(std::string_view mesh_name, bool visible, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	std::string drawable_name("Drawable" + std::to_string(m_rbo.drawables.size()) + "_" + mesh_name.data());
	m_rbo.add_drawable(mesh_name, drawable_name, visible, position, rotation, scale);
}

void SampleApp::Initialize()
{
	InitSceneResources();

	m_light_manager.init();
	m_model_renderer.reset(new VulkanModelRenderer);
	m_imgui_renderer.reset(new VulkanImGuiRenderer(context));
	
	m_cascaded_shadow_renderer.init(2048, 2048, m_Camera, m_light_manager);
	m_cubemap_renderer.init(); //!\ Before deferred renderer

	m_deferred_renderer.init(
		m_cascaded_shadow_renderer.m_shadow_maps,
		m_light_manager
	);

	m_imgui_renderer->init(m_shadow_renderer);
	m_postprocess.init();
	m_postprocess.m_postfx_downsample.init();
}

void SampleApp::Update(float dt)
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();

	if (EngineGetAsyncKeyState(Key::Z))     m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::FORWARD;
	if (EngineGetAsyncKeyState(Key::Q))     m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::LEFT;
	if (EngineGetAsyncKeyState(Key::S))     m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::BACKWARD;
	if (EngineGetAsyncKeyState(Key::D))     m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::RIGHT;
	if (EngineGetAsyncKeyState(Key::SPACE)) m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::UP;
	if (EngineGetAsyncKeyState(Key::LSHIFT)) m_Camera.controller.m_movement = m_Camera.controller.m_movement | Movement::DOWN;

	m_light_manager.update(nullptr, m_light_manager.view_volume_bbox_min, m_light_manager.view_volume_bbox_max);
	
	UpdateRenderersData(dt, context.curr_frame_idx);
}

void SampleApp::Render()
{
	const uint32_t& frame_idx = context.curr_frame_idx;
	VulkanFrame current_frame = context.frames[frame_idx];

	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VK_CHECK(vkWaitForFences(context.device, 1, &current_frame.fence_queue_submitted, true, OneSecondInNanoSeconds));
	VK_CHECK(vkResetFences(context.device, 1, &current_frame.fence_queue_submitted));

	uint32_t swapchain_buffer_idx = 0;
	VkResult result = swapchain.AcquireNextImage(current_frame.smp_image_acquired, &swapchain_buffer_idx);

	VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkResetCommandPool(context.device, current_frame.cmd_pool, 0));
	VK_CHECK(vkBeginCommandBuffer(current_frame.cmd_buffer, &cmdBufferBeginInfo));
	
	/* Transition to color attachment */
	swapchain.color_attachments[frame_idx].transition_layout(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	{
		m_model_renderer->render(frame_idx, current_frame.cmd_buffer);
		m_cascaded_shadow_renderer.render(frame_idx, current_frame.cmd_buffer);
		m_deferred_renderer.render(frame_idx, current_frame.cmd_buffer);
		m_postprocess.m_postfx_downsample.render(current_frame.cmd_buffer);
		m_cubemap_renderer.render_skybox(frame_idx, current_frame.cmd_buffer);
		m_imgui_renderer->render(frame_idx, current_frame.cmd_buffer);
	}
	
	/* Transition to present */
	swapchain.color_attachments[frame_idx].transition_layout(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	VK_CHECK(vkEndCommandBuffer(current_frame.cmd_buffer));

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
	m_rbo.update_per_object_data(frame_data);

	m_deferred_renderer.update(currentImageIdx);
	m_model_renderer->update(currentImageIdx, frame_data);

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
		m_UI.ShowLightSettings(&m_light_manager);
		m_UI.ShowShadowPanel(&m_cascaded_shadow_renderer, m_imgui_renderer->m_ShadowCascadesTextureIds[currentImageIdx]);
		m_UI.End();
	}

	{
		m_imgui_renderer->update_buffers(ImGui::GetDrawData());
	}

	/* Temp */
	//m_rbo.get_drawable("frustum")->transform.model  = glm::inverse(m_light_manager.proj * m_light_manager.view);


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
	/* Update camera */
	Application::OnMouseMove(x, y);
	if (m_UI.bIsSceneViewportHovered)
	{
		if (m_MouseEvent.bLeftClick)
		{
			m_MouseEvent.lastClickPosX = x;
			m_MouseEvent.lastClickPosY = y;
			if (m_MouseEvent.bFirstClick)
			{
				m_MouseEvent.bFirstClick = false;
			}
			else
			{
				m_Camera.controller.UpdateRotation( { x, y });
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

