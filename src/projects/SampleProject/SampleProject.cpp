#include "SampleProject.h"

#include "engine/Window.h"
#include "rendering/vulkan/VulkanDebugUtils.h"
#include "rendering/vulkan/VulkanRenderInterface.h"
#include "rendering/vulkan/RenderObjectManager.h"

#include "glm/gtx/quaternion.hpp"

#include "rendering/vulkan/Renderers/DebugLineRenderer.hpp"
#include "rendering/vulkan/Renderers/DeferredRenderer.hpp"
#include "rendering/vulkan/Renderers/ForwardRenderer.hpp"
#include "rendering/vulkan/Renderers/SkyboxRenderer.hpp"

static ForwardRenderer forward_renderer;
static DebugLineRenderer debug_line_renderer;
static DeferredRenderer deferred_renderer;
static SkyboxRenderer skybox_renderer;
static IBLRenderer ibl_renderer;
static bool using_skybox = true;

static std::vector<size_t> drawable_list;

SampleProject::SampleProject(const char* title, uint32_t width, uint32_t height)
	: Application(title, width, height)
{
	
}

void SampleProject::init()
{
	ObjectManager::get_instance().init();

	debug_line_renderer.init();
	forward_renderer.p_debug_line_renderer = &debug_line_renderer;
	forward_renderer.init();

	ibl_renderer.init("blaubeuren_night_1k.hdr");
	skybox_renderer.init();
	deferred_renderer.init();
	skybox_renderer.init(cubemap_renderer.cubemap_attachment);
	m_camera.update_aspect_ratio(m_gui.scene_view_aspect_ratio);

	create_scene();
}

void SampleProject::compose_gui()
{
	m_gui.begin();
	m_gui.show_viewport_window(m_camera);
	m_gui.show_toolbar();
	m_gui.show_gizmo(m_camera, m_event_manager->key_event, ObjectManager::get_instance().m_mesh_instance_data[0][0].model);
	m_gui.show_inspector(ObjectManager::get_instance());
	m_gui.show_camera_settings(m_camera);
	m_gui.show_draw_statistics(IRenderer::draw_stats);
	m_gui.show_shader_library();
	deferred_renderer.show_ui();
	ibl_renderer.show_ui();
	m_gui.end();
}

void SampleProject::update_frame_ubo()
{
	VulkanRendererCommon::FrameData frame_data;
	frame_data.view =  m_camera.get_view();
	frame_data.proj = m_camera.get_proj();
	frame_data.view_proj = m_camera.get_proj() * m_camera.get_view();
	frame_data.view_proj_inv = glm::inverse(frame_data.view_proj);
	frame_data.camera_pos_ws = glm::vec4(m_camera.controller.m_position, 1);
	frame_data.time = m_time;
	VulkanRendererCommon::get_instance().update_frame_data(frame_data, context.curr_frame_idx);
}

void SampleProject::update(float t, float dt)
{
	/* Update camera */
	if (m_window->is_in_focus())
	{
		if (m_gui.is_scene_viewport_active())
		{
			if (m_event_manager->mouse_event.b_lmb_click)
			{
				m_camera.rotate(m_delta_time, m_event_manager->mouse_event);
			}
		}
		m_camera.translate(m_delta_time, m_event_manager->key_event);
	}


	static ObjectManager& object_manager = ObjectManager::get_instance();

	/* Update CPU scene data */
	std::string mesh_name = "mesh_1";
	size_t mesh_idx = object_manager.m_mesh_id_from_name.at(mesh_name);

	//for (size_t instance_idx = 0; instance_idx < object_manager.m_mesh_instance_data[mesh_idx].size(); instance_idx++)
	//{
	//	float normalized_index = (instance_idx / float(object_manager.m_mesh_instance_data[mesh_idx].size())) * 2 - 1;
	//	
	//	object_manager.m_mesh_instance_data[mesh_idx][instance_idx].model = glm::rotate(object_manager.m_mesh_instance_data[mesh_idx][instance_idx].model, dt, glm::vec3(0,1, 0));
	//}

	compose_gui();
}

void SampleProject::render()
{
	VkCommandBuffer& cmd_buffer = context.get_current_frame().cmd_buffer;
	IRenderer::draw_stats.reset();
	
	set_polygon_mode(cmd_buffer, IRenderer::global_polygon_mode);
	set_viewport_scissor(cmd_buffer, context.swapchain->info.width, context.swapchain->info.height, true);

	context.swapchain->clear_color(cmd_buffer);

	deferred_renderer.render(cmd_buffer, drawable_list);
	skybox_renderer.render(cmd_buffer);
	//forward_renderer.render(cmd_buffer, drawable_list);
	//debug_line_renderer.render(cmd_buffer);
	m_gui.render(cmd_buffer);

	update_gpu_buffers();
}

void SampleProject::update_gpu_buffers()
{
	update_instances_ssbo();
	update_frame_ubo();
}

void SampleProject::create_scene()
{
	VulkanMesh mesh;
	//mesh.create_from_file("basic/unit_sphere.glb");
	//mesh.create_from_file("basic/unit_cube.glb");
	//mesh.create_from_file("scenes/goggles/scene.gltf");
	//mesh.create_from_file("scenes/sponza-gltf-pbr/scene.glb");
	//mesh.create_from_file("basic/armor/scene.gltf");
	//mesh.create_from_file("basic/lantern/scene.gltf");
	//mesh.create_from_file("basic/lantern/scene.gltf");

	//mesh.create_from_file("scenes/bistro/scene.gltf");
	//ObjectManager::get_instance().add_mesh(mesh, "mesh", { .position = { 0,0,0 }, .rotation = {0,0,0}, .scale = { 0.1f, 0.1f, 0.1f } });

	VulkanMesh mesh_1; 
	mesh_1.create_from_file("basic/cobblestone/scene.gltf");
	drawable_list.push_back(ObjectManager::get_instance().add_mesh(mesh_1, "mesh_1", { .position = { 0,0,0 }, .rotation = {0,0,0}, .scale = {  1.0f, 1.0f, 1.0f  } }));

	//for (int x = -15; x < 15; x++)
	//for (int y = -15; y < 15; y++)
	//for (int z = -15; z < 15; z++)
	//{
	//	float spacing = 10.0f;
	//	Transform t = { .position = spacing * glm::vec3(x, y, z), .rotation = {0,0,0}, .scale = glm::vec3{ 1.0f, 1.0f, 1.0f } };

	//	//if ((x + z + 0) % 2 == 0)
	//	{
	//		ObjectManager::get_instance().add_mesh_instance("mesh_1", ObjectManager::GPUInstanceData{ .model = glm::mat4(t), .color = glm::vec4(glm::sphericalRand(1.0f), 1.0f) });
	//	}
	//	//else
	//	//{
	//	//	ObjectManager::get_instance().add_mesh_instance("mesh_2", ObjectManager::GPUInstanceData{ .model = glm::mat4(t), .color = glm::vec4(glm::sphericalRand(1.0f), 1.0f)});
	//	//}
	//}
}

void SampleProject::update_instances_ssbo()
{
	static ObjectManager& object_manager = ObjectManager::get_instance();

	object_manager.update_instances_ssbo("mesh_1");
	//object_manager.update_instances_ssbo("mesh_2");
}

void SampleProject::exit()
{
	m_gui.exit();
}

void SampleProject::on_window_resize()
{
	Application::on_window_resize();
	m_camera.update_aspect_ratio(m_window->GetWidth() / (float)m_window->GetHeight());
	m_gui.on_window_resize();
	forward_renderer.on_window_resize();
	debug_line_renderer.on_window_resize();
}

void SampleProject::on_lmb_down(MouseEvent event)
{
	m_camera.controller.b_first_click = true;
    m_camera.controller.m_last_mouse_pos = { (float)event.px, (float)event.py };
}

void SampleProject::on_mouse_move(MouseEvent event)
{
	Application::on_mouse_move(event);
}

void SampleProject::on_key_event(KeyEvent event)
{
	Application::on_key_event(event);

	if (event.key == Key::R)
	{

	}
}

