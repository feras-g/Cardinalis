#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "core/engine/Application.h"
#include "core/engine/Window.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanShader.h"

#include "core/rendering/vulkan/VulkanUI.h"
#include "core/rendering/Camera.h"
#include "core/rendering/FrameCounter.h"
#include "core/rendering/vulkan/LightManager.h"

#include "core/rendering/vulkan/Renderers/VulkanImGuiRenderer.h"
#include "core/rendering/vulkan/Renderers/VulkanModelRenderer.h"
#include "core/rendering/vulkan/Renderers/DeferredRenderer.h"
#include "core/rendering/vulkan/Renderers/ShadowRenderer.h"
#include "core/rendering/vulkan/Renderers/CubemapRenderer.h"
#include "core/rendering/vulkan/Renderers/PostProcessingRenderer.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 2560, 1440)
	{

	}
	~SampleApp();
	void Initialize() override;
	void InitSceneResources();
	void LoadMesh(std::string_view path, std::string_view name);
	void AddDrawable(std::string_view mesh_name, DrawFlag flags,
	                 glm::vec3 position = {0,0,0}, glm::vec3 rotation = {0,0,0}, glm::vec3 scale = {1,1,1});
	void Update(float t, float dt) override;
	void Render() override;
	void UpdateRenderersData(float dt, size_t currentImageIdx) override;
	void Terminate() override;

	void OnWindowResize() override;
	void OnLeftMouseButtonUp() override;
	void OnLeftMouseButtonDown() override;
	void OnMouseMove(int x, int y) override;
	void OnKeyEvent(KeyEvent event);

protected:
	VulkanUI m_ui;
	Camera m_camera;
	LightManager m_light_manager;
	RenderObjectManager m_object_manager;

protected:
	std::unique_ptr<VulkanImGuiRenderer>		m_imgui_renderer;
	std::unique_ptr<VulkanModelRenderer>		m_model_renderer;
	DeferredRenderer m_deferred_renderer;
	CascadedShadowRenderer m_cascaded_shadow_renderer;
	CubemapRenderer m_cubemap_renderer;
	PostProcessRenderer m_postprocess;
};

void SampleApp::InitSceneResources()
{
	CameraController fpsController = CameraController( { 10, 1, 0 }, { 0, -90, 0 }, { 0, 0, 1 }, { 0, -1, 0 });
	m_camera = Camera(fpsController, 45.0f, 1.0f, 0.5f, 250.0f);

	m_object_manager.init();

	/* Basic primitives */
	{
		VulkanMesh mesh;
		mesh.create_from_file("../../../data/models/basic/unit_cube.glb");
		uint32_t id = (uint32_t)m_object_manager.add_mesh(mesh, "cube");
		m_object_manager.add_drawable(id, "skybox",			  DrawFlag::NONE);
		m_object_manager.add_drawable(id, "placeholder_cube", DrawFlag::NONE);
	}

	LoadMesh("../../../data/models/basic/unit_plane.glb", "Plane");
	LoadMesh("../../../data/models/basic/column_5m.glb",  "Column");
	LoadMesh("../../../data/models/basic/unit_sphere.glb", "Sphere");
	LoadMesh("../../../data/models/test/metalroughspheres/MetalRoughSpheres.gltf", "MetalRoughSpheres");

	//LoadMesh("../../../data/models/scenes/Powerplant_GLTF/powerplant.gltf", "Powerplant");
	//LoadMesh("../../../data/models/scenes/bistro_lit/bistro_lights.gltf", "Bistro");
	//LoadMesh("../../../data/models/scenes/sponza-gltf-pbr/sponza.glb", "Sponza");
	//LoadMesh("../../../data/models/scenes/tree/scene.gltf", "Tree");
	//LoadMesh("../../../data/models/scenes/subway/scene.gltf", "Subway");


	//LoadMesh("../../../data/models/test/metalroughspheres/MetalRoughSpheres.gltf", "MetalRoughSpheres");
	LoadMesh("../../../data/models/basic/duck/duck.gltf", "ColladaDuck");
	//LoadMesh("../../../data/models/scenes/book/scene.gltf", "Book");
	//LoadMesh("../../../data/models/scenes/building/scene.gltf", "Building");


	//glm::ivec3 dimensions(1,1,20);
	//float spacing = 20;
	//for (int x = 0; x < dimensions.x; x++)
	//{
	//	for (int y = 0; y < dimensions.y; y++)
	//	{
	//		for (int z = 0; z < dimensions.z; z++)
	//		{
	//			glm::vec3 position{ x, y, z };
	//			AddDrawable("Column", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, position * spacing, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
	//		}
	//	}
	//}

	//AddDrawable("Powerplant", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 0.10f, 0.10f, 0.10f });
	AddDrawable("Sphere", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, {0, 5, 0}, {0, 0, 0}, {1, 1, 1});
	AddDrawable("Floor",  DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, -5, 0 }, { 0, 0, 0 }, { 100.0f, 1.0f, 100.0f });
	AddDrawable("MetalRoughSpheres", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 });
	AddDrawable("ColladaDuck", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1,1,1 });
	//AddDrawable("Sponza", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
	//AddDrawable("Book", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
	//AddDrawable("Building", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });


	//AddDrawable("Bistro", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
	//AddDrawable("Subway", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
	
	//AddDrawable("Tree", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1,1,1 });

	m_object_manager.configure();


	/* Temp: add manually point lights */
	//VulkanMesh* bistro = m_object_manager.get_mesh("Bistro");
	//for (int i=0; i < bistro->geometry_data.punctual_lights_positions.size(); i++)
	//{
	//	m_light_manager.light_data.point_lights.push_back
	//	(
	//		{
	//			bistro->geometry_data.punctual_lights_positions[i],
	//			bistro->geometry_data.punctual_lights_colors[i],
	//			1.0
	//		}
	//	);
	//}
	
}

inline void SampleApp::LoadMesh(std::string_view path, std::string_view name)
{
	VulkanMesh mesh;
	mesh.create_from_file(path.data());
	uint32_t id = (uint32_t)m_object_manager.add_mesh(mesh, name.data());
}


inline void SampleApp::AddDrawable(std::string_view mesh_name, DrawFlag flags, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	std::string drawable_name(mesh_name.data());
	m_object_manager.add_drawable(mesh_name, drawable_name, flags, position, rotation, scale);
}

void SampleApp::Initialize()
{
	InitSceneResources();
	m_light_manager.init();

	m_model_renderer.reset(new VulkanModelRenderer);
	m_imgui_renderer.reset(new VulkanImGuiRenderer(context));
	
	m_cascaded_shadow_renderer.init(2048, 2048, m_camera, m_light_manager);
	m_cubemap_renderer.init(); //!\ Before deferred renderer

	m_deferred_renderer.init(
		m_cascaded_shadow_renderer.m_shadow_maps,
		m_light_manager
	);

	m_imgui_renderer->init();
	//m_postprocess.init();
	//m_postprocess.m_postfx_downsample.init();
}

void SampleApp::Update(float t, float dt)
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();

	if (m_Window->is_in_focus())
	{
		if (EngineGetAsyncKeyState(Key::Z))     m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::FORWARD;
		if (EngineGetAsyncKeyState(Key::Q))     m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::LEFT;
		if (EngineGetAsyncKeyState(Key::S))     m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::BACKWARD;
		if (EngineGetAsyncKeyState(Key::D))     m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::RIGHT;
		if (EngineGetAsyncKeyState(Key::SPACE)) m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::UP;
		if (EngineGetAsyncKeyState(Key::LSHIFT)) m_camera.controller.m_movement = m_camera.controller.m_movement | Movement::DOWN;
	}

	m_light_manager.update(t, dt);

	UpdateRenderersData(dt, context.curr_frame_idx);

	// TODO : Remove temporary reload shader thingy
	if (EngineGetAsyncKeyState(Key::R))
	{
		m_deferred_renderer.reload_shader();
	}
}

void SampleApp::Render()
{
	const uint32_t& frame_idx = context.curr_frame_idx;
	VulkanFrame current_frame = context.frames[frame_idx];

	VulkanSwapchain& swapchain = *m_RHI->get_swapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VK_CHECK(vkWaitForFences(context.device, 1, &current_frame.fence_queue_submitted, true, 1000000000ull));
	VK_CHECK(vkResetFences(context.device, 1, &current_frame.fence_queue_submitted));

	uint32_t swapchain_buffer_idx = 0;
	VkResult result = swapchain.AcquireNextImage(current_frame.smp_image_acquired, &swapchain_buffer_idx);

	VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_CHECK(vkResetCommandPool(context.device, current_frame.cmd_pool, 0));
	VK_CHECK(vkBeginCommandBuffer(current_frame.cmd_buffer, &cmdBufferBeginInfo));
	
	/* Transition to color attachment */
	swapchain.color_attachments[swapchain_buffer_idx].transition(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	
	m_model_renderer->render(frame_idx, current_frame.cmd_buffer);
	m_cascaded_shadow_renderer.render(frame_idx, current_frame.cmd_buffer);
	m_deferred_renderer.render(frame_idx, current_frame.cmd_buffer);
	//m_postprocess.m_postfx_downsample.render(current_frame.cmd_buffer);
	m_cubemap_renderer.render_skybox(frame_idx, current_frame.cmd_buffer);
	m_imgui_renderer->render(frame_idx, current_frame.cmd_buffer);
	
	/* Transition to present */
	swapchain.color_attachments[swapchain_buffer_idx].transition(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_NONE);
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
	VkPresentInfoKHR present_info = {};
	
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &current_frame.smp_queue_submitted;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain.swapchain;
	present_info.pImageIndices = &swapchain_buffer_idx;
	

	vkQueuePresentKHR(context.queue, &present_info);

	context.frame_count++;
	context.curr_frame_idx = (context.curr_frame_idx + 1) % NUM_FRAMES;
}

inline void SampleApp::UpdateRenderersData(float dt, size_t currentImageIdx)
{
	if (m_ui.default_scene_view_aspect_ratio != m_camera.aspect_ratio)
	{
		m_camera.UpdateAspectRatio(m_ui.default_scene_view_aspect_ratio);
	}
	m_camera.controller.deltaTime = dt;
	m_camera.controller.UpdateTranslation(dt);

	/* Update frame Data */
	VulkanRendererBase::PerFrameData frame_data;
	{
		frame_data.view = m_camera.GetView();
		frame_data.proj = m_camera.GetProj();
		frame_data.inv_view_proj = glm::inverse(frame_data.proj * frame_data.view);
		frame_data.view_pos = glm::vec4(
			m_camera.controller.m_position.x,
			m_camera.controller.m_position.y,
			m_camera.controller.m_position.z,
			1.0);
		VulkanRendererBase::get_instance().update_frame_data(frame_data, currentImageIdx);
	}

	/* Update drawables */
	m_object_manager.update_per_object_data(frame_data);

	m_model_renderer->update(currentImageIdx, frame_data);

	// ImGui composition
	{
		m_ui.Start();
		//m_UI.AddHierarchyPanel();
		//m_UI.ShowMenuBar();
		m_ui.AddInspectorPanel();
		m_ui.ShowStatistics(m_DebugName, dt, context.frame_count);
		m_ui.ShowSceneViewportPanel(m_camera,
			m_imgui_renderer->m_DeferredRendererOutputTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererColorTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererNormalTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererDepthTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererNormalMapTextureId[currentImageIdx],
			m_imgui_renderer->m_ModelRendererMetallicRoughnessTextureId[currentImageIdx]
		);
		m_ui.ShowCameraSettings(&m_camera);
		m_ui.ShowInspector();
		m_ui.ShowLightSettings(&m_light_manager);
		m_ui.ShowShadowPanel(&m_cascaded_shadow_renderer, m_imgui_renderer->m_CascadedShadowRenderer_Textures[currentImageIdx]);
		m_ui.End();
	}

	//{
	//	m_imgui_renderer->update_buffers(ImGui::GetDrawData());
	//}

	/* Temp */
	//m_rbo.get_drawable("frustum")->transform.model  = glm::inverse(m_light_manager.proj * m_light_manager.view);


}

inline void SampleApp::OnWindowResize()
{
	context.swapchain->Reinitialize();
	m_imgui_renderer->update_render_pass_attachments();
}

inline void SampleApp::OnLeftMouseButtonUp()
{
	Application::OnLeftMouseButtonUp();
}

void SampleApp::OnLeftMouseButtonDown()
{
	Application::OnLeftMouseButtonDown();
	m_camera.controller.m_last_mouse_pos = { m_MouseEvent.px, m_MouseEvent.py };
}

inline void SampleApp::OnMouseMove(int x, int y)
{
	/* Update camera */
	Application::OnMouseMove(x, y);
	if (m_ui.is_scene_viewport_hovered)
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
				m_camera.controller.UpdateRotation( { x, y });
			}
		}
	}
}

inline void SampleApp::OnKeyEvent(KeyEvent event)
{

}

inline void SampleApp::Terminate()
{
	vkDeviceWaitIdle(context.device);
	m_imgui_renderer->destroy();
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{

}

#endif // !SAMPLE_APP_H

