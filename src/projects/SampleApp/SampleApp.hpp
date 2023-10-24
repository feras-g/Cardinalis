//#ifndef SAMPLE_APP_H
//#define SAMPLE_APP_H
//
//#include "core/engine/Application.h"
//#include "core/engine/Window.h"
//#include "core/rendering/vulkan/VulkanRenderInterface.h"
//#include "core/rendering/vulkan/VulkanShader.h"
//
//#include "core/rendering/vulkan/VulkanUI.h"
//#include "core/rendering/Camera.h"
//#include "core/rendering/FrameCounter.h"
//#include "core/rendering/vulkan/LightManager.h"
//
//#include "core/rendering/vulkan/Renderers/VulkanImGuiRenderer.h"
//#include "core/rendering/vulkan/Renderers/VulkanModelRenderer.h"
//#include "core/rendering/vulkan/Renderers/DeferredRenderer.h"
//#include "core/rendering/vulkan/Renderers/ShadowRenderer.h"
//#include "core/rendering/vulkan/Renderers/CubemapRenderer.h"
//#include "core/rendering/vulkan/Renderers/PostProcessingRenderer.h"
//
//class SampleApp final : public Application
//{
//public:
//	SampleApp() : Application("SampleApp", 800, 600)
//	{
//
//	}
//	~SampleApp();
//	void init() override;
//	void InitSceneResources();
//	void LoadMesh(std::string_view path, std::string_view name);
//	void AddDrawable(std::string_view mesh_name, DrawFlag flags,
//	                 glm::vec3 position = {0,0,0}, glm::vec3 rotation = {0,0,0}, glm::vec3 scale = {1,1,1});
//	void update(float t, float dt) override;
//	void render() override;
//	void compose_gui() override;
//	void UpdateRenderersData(float dt, size_t currentImageIdx);
//	void exit() override;
//
//	void on_window_resize() override;
//	void on_lmb_up() override;
//	void on_lmb_down() override;
//	void on_mouse_move(int x, int y) override;
//	void on_key_event(KeyEvent event);
//
//protected:
//	VulkanGUI m_ui;
//	Camera m_camera;
//	LightManager m_light_manager;
//	RenderObjectManager m_object_manager;
//
//protected:
//	
//	std::unique_ptr<VulkanModelRenderer>		m_model_renderer;
//	DeferredRenderer m_deferred_renderer;
//	CascadedShadowRenderer m_cascaded_shadow_renderer;
//	CubemapRenderer m_cubemap_renderer;
//	PostProcessRenderer m_postprocess;
//};
//
//void SampleApp::InitSceneResources()
//{
//	CameraController fpsController = CameraController( { 10, 1, 0 }, { 0, -90, 0 }, { 0, 0, 1 }, { 0, -1, 0 });
//	m_camera = Camera(fpsController, 45.0f, 1.0f, 0.5f, 250.0f);
//
//	m_object_manager.init();
//
//	/* Basic primitives */
//	{
//		VulkanMesh mesh;
//		mesh.create_from_file("../../../data/models/basic/unit_cube.glb");
//		uint32_t id = (uint32_t)m_object_manager.add_mesh(mesh, "cube");
//		m_object_manager.add_drawable(id, "skybox",			  DrawFlag::NONE);
//		m_object_manager.add_drawable(id, "placeholder_cube", DrawFlag::NONE);
//	}
//
//	LoadMesh("../../../data/models/basic/unit_plane.glb", "Plane");
//	LoadMesh("../../../data/models/basic/column_5m.glb",  "Column");
//	LoadMesh("../../../data/models/basic/unit_sphere.glb", "Sphere");
//	LoadMesh("../../../data/models/test/metalroughspheres/MetalRoughSpheres.gltf", "MetalRoughSpheres");
//
//	//LoadMesh("../../../data/models/scenes/Powerplant_GLTF/powerplant.gltf", "Powerplant");
//	//LoadMesh("../../../data/models/scenes/bistro_lit/bistro_lights.gltf", "Bistro");
//	//LoadMesh("../../../data/models/scenes/sponza-gltf-pbr/sponza.glb", "Sponza");
//	//LoadMesh("../../../data/models/scenes/tree/scene.gltf", "Tree");
//	//LoadMesh("../../../data/models/scenes/subway/scene.gltf", "Subway");
//
//
//	//LoadMesh("../../../data/models/test/metalroughspheres/MetalRoughSpheres.gltf", "MetalRoughSpheres");
//	LoadMesh("../../../data/models/basic/duck/duck.gltf", "ColladaDuck");
//	//LoadMesh("../../../data/models/scenes/book/scene.gltf", "Book");
//	//LoadMesh("../../../data/models/scenes/building/scene.gltf", "Building");
//
//
//	//glm::ivec3 dimensions(1,1,20);
//	//float spacing = 20;
//	//for (int x = 0; x < dimensions.x; x++)
//	//{
//	//	for (int y = 0; y < dimensions.y; y++)
//	//	{
//	//		for (int z = 0; z < dimensions.z; z++)
//	//		{
//	//			glm::vec3 position{ x, y, z };
//	//			AddDrawable("Column", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, position * spacing, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//	//		}
//	//	}
//	//}
//
//	//AddDrawable("Powerplant", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 0.10f, 0.10f, 0.10f });
//	AddDrawable("Sphere", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, {0, 5, 0}, {0, 0, 0}, {1, 1, 1});
//	AddDrawable("Floor",  DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, -5, 0 }, { 0, 0, 0 }, { 100.0f, 1.0f, 100.0f });
//	AddDrawable("MetalRoughSpheres", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 });
//	AddDrawable("ColladaDuck", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1,1,1 });
//	//AddDrawable("Sponza", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//	//AddDrawable("Book", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//	//AddDrawable("Building", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//
//
//	//AddDrawable("Bistro", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//	//AddDrawable("Subway", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
//	
//	//AddDrawable("Tree", DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW, { 0, 0, 0 }, { 0, 0, 0 }, { 1,1,1 });
//
//	m_object_manager.configure();
//
//
//	/* Temp: add manually point lights */
//	//VulkanMesh* bistro = m_object_manager.get_mesh("Bistro");
//	//for (int i=0; i < bistro->geometry_data.punctual_lights_positions.size(); i++)
//	//{
//	//	m_light_manager.light_data.point_lights.push_back
//	//	(
//	//		{
//	//			bistro->geometry_data.punctual_lights_positions[i],
//	//			bistro->geometry_data.punctual_lights_colors[i],
//	//			1.0
//	//		}
//	//	);
//	//}
//	
//}
//
//inline void SampleApp::LoadMesh(std::string_view path, std::string_view name)
//{
//	VulkanMesh mesh;
//	mesh.create_from_file(path.data());
//	uint32_t id = (uint32_t)m_object_manager.add_mesh(mesh, name.data());
//}
//
//
//inline void SampleApp::AddDrawable(std::string_view mesh_name, DrawFlag flags, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
//{
//	std::string drawable_name(mesh_name.data());
//	m_object_manager.add_drawable(mesh_name, drawable_name, flags, position, rotation, scale);
//}
//
//void SampleApp::init()
//{
//	VulkanRendererBase& renderer_base = VulkanRendererBase::get_instance();
//
//	renderer_base.create_samplers();
//	renderer_base.create_buffers();
//	renderer_base.create_attachments();
//	renderer_base.create_descriptor_sets();
//
//	InitSceneResources();
//	m_light_manager.init();
//
//	m_model_renderer.reset(new VulkanModelRenderer);
//	
//	m_cascaded_shadow_renderer.init(2048, 2048, m_camera, m_light_manager);
//	m_cubemap_renderer.init(); //!\ Before deferred renderer
//
//	m_deferred_renderer.init(
//		m_cascaded_shadow_renderer.m_shadow_maps,
//		m_light_manager
//	);
//
//	//m_postprocess.init();
//	//m_postprocess.m_postfx_downsample.init();
//}
//
//void SampleApp::update(float t, float dt)
//{
//	// Update keyboard, mouse interaction
//	m_window->update();
//
//	if (m_window->is_in_focus())
//	{
//		m_camera.controller.update(dt, *this);
//	}
//
//	m_light_manager.update(t, dt);
//
//	UpdateRenderersData(dt, context.curr_frame_idx);
//
//	// TODO : Remove temporary reload shader thingy
//	if (get_key_state(Key::R))
//	{
//		m_deferred_renderer.reload_shader();
//	}
//}
//
//void SampleApp::render()
//{
//	uint32_t frame_idx = context.curr_frame_idx;
//	const VulkanFrame& frame = context.frames[frame_idx];
//
//	m_model_renderer->render(frame_idx, frame.cmd_buffer);
//	m_cascaded_shadow_renderer.render(frame_idx, frame.cmd_buffer);
//	m_deferred_renderer.render(frame_idx, frame.cmd_buffer);
//	//m_postprocess.m_postfx_downsample.render(frame.cmd_buffer);
//	m_cubemap_renderer.render_skybox(frame_idx, frame.cmd_buffer);
//	m_ui.render(frame.cmd_buffer);
//}
//
//inline void SampleApp::compose_gui()
//{
//	m_ui.begin();
//	//m_UI.AddHierarchyPanel();
//	//m_UI.ShowMenuBar();
//	//m_ui.AddInspectorPanel();
//	//m_ui.ShowStatistics(m_debug_name, get_time_secs(), context.frame_count);
//	//m_ui.ShowSceneViewportPanel(m_camera,
//	//	m_imgui_renderer->m_DeferredRendererOutputTextureId[context.curr_frame_idx],
//	//	m_imgui_renderer->m_ModelRendererColorTextureId[context.curr_frame_idx],
//	//	m_imgui_renderer->m_ModelRendererNormalTextureId[context.curr_frame_idx],
//	//	m_imgui_renderer->m_ModelRendererDepthTextureId[context.curr_frame_idx],
//	//	m_imgui_renderer->m_ModelRendererNormalMapTextureId[context.curr_frame_idx],
//	//	m_imgui_renderer->m_ModelRendererMetallicRoughnessTextureId[context.curr_frame_idx]
//	//);
//	//m_ui.ShowCameraSettings(&m_camera);
//	//m_ui.ShowInspector();
//	//m_ui.ShowLightSettings(&m_light_manager);
//	//m_ui.ShowShadowPanel(&m_cascaded_shadow_renderer, m_imgui_renderer->m_CascadedShadowRenderer_Textures[context.curr_frame_idx]);
//	m_ui.end();
//}
//
//inline void SampleApp::UpdateRenderersData(float dt, size_t currentImageIdx)
//{
//	if (m_ui.default_scene_view_aspect_ratio != m_camera.aspect_ratio)
//	{
//		m_camera.update_aspect_ratio(m_ui.default_scene_view_aspect_ratio);
//	}
//	m_camera.controller.deltaTime = dt;
//	m_camera.controller.update(dt);
//
//	/* Update frame Data */
//	VulkanRendererBase::FrameData frame_data;
//	{
//		frame_data.view = m_camera.get_view();
//		frame_data.proj = m_camera.get_proj();
//		frame_data.inv_view_proj = glm::inverse(frame_data.proj * frame_data.view);
//		frame_data.view_pos = glm::vec4(
//			m_camera.controller.m_position.x,
//			m_camera.controller.m_position.y,
//			m_camera.controller.m_position.z,
//			1.0);
//		VulkanRendererBase::get_instance().update_frame_data(frame_data, currentImageIdx);
//	}
//
//	/* Update drawables */
//	m_object_manager.update_per_object_data(frame_data);
//
//	m_model_renderer->update(currentImageIdx, frame_data);
//
//	// ImGui composition
//	compose_gui();
//}
//
//void SampleApp::on_window_resize()
//{
//	Application::on_window_resize();
//	m_ui.on_window_resize();
//}
//
//void SampleApp::on_lmb_up()
//{
//	Application::on_lmb_up();
//}
//
//void SampleApp::on_lmb_down()
//{
//	Application::on_lmb_down();
//	m_camera.controller.m_last_mouse_pos = { m_mouse_event.px, m_mouse_event.py };
//}
//
//inline void SampleApp::on_mouse_move(int x, int y)
//{
//	/* Update camera */
//	Application::on_mouse_move(x, y);
//	if (m_ui.is_scene_viewport_hovered)
//	{
//
//	}
//}
//
//inline void SampleApp::on_key_event(KeyEvent event)
//{
//
//}
//
//inline void SampleApp::exit()
//{
//	vkDeviceWaitIdle(context.device);
//	m_window->ShutdownGUI();
//}
//
//inline SampleApp::~SampleApp()
//{
//
//}
//
//#endif // !SAMPLE_APP_H
//
