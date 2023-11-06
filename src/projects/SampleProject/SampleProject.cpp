#include "SampleProject.h"

#include "engine/Window.h"
#include "rendering/vulkan/VulkanDebugUtils.h"
#include "rendering/vulkan/VulkanRenderInterface.h"
#include "rendering/vulkan/RenderObjectManager.h"

#include "glm/gtx/quaternion.hpp"

#include "rendering/vulkan/Renderers/DebugLineRenderer.hpp"
#include "rendering/vulkan/Renderers/ForwardRenderer.hpp"

static ForwardRenderer forward_renderer;
static DebugLineRenderer debug_line_renderer;

SampleProject::SampleProject(const char* title, uint32_t width, uint32_t height)
	: Application(title, width, height)
{
	m_camera.update_aspect_ratio(m_window->GetAspectRatio());
}

void SampleProject::init()
{
	ObjectManager::get_instance().init();

	debug_line_renderer.init();
	forward_renderer.p_debug_line_renderer = &debug_line_renderer;
	forward_renderer.init();

	create_scene();
}

void SampleProject::compose_gui()
{
	m_gui.begin();
	m_gui.show_gizmo(m_camera, m_event_manager->key_event, ObjectManager::get_instance().m_mesh_instance_data[0][0].model);
	m_gui.show_inspector(ObjectManager::get_instance());
	m_gui.show_camera_settings(m_camera);
	m_gui.end();
}

void SampleProject::update_frame_data()
{
	VulkanRendererCommon::FrameData frame_data;
	frame_data.view_proj     = m_camera.get_proj() * m_camera.get_view();
	frame_data.view_proj_inv = glm::inverse(frame_data.view_proj);
	frame_data.camera_pos_ws = glm::vec4(m_camera.controller.m_position, 1);
	frame_data.time = m_time;
	VulkanRendererCommon::get_instance().update_frame_data(frame_data, context.curr_frame_idx);
}

void SampleProject::update(float t, float dt)
{
	update_frame_data();
	compose_gui();
	update_scene(t, dt);
}

void SampleProject::render()
{
	VkCommandBuffer& cmd_buffer = context.get_current_frame().cmd_buffer;
	set_viewport_scissor(cmd_buffer, context.swapchain->info.width, context.swapchain->info.height, true);

	forward_renderer.render(cmd_buffer, ObjectManager::get_instance());
	//debug_line_renderer.render(cmd_buffer);

	m_gui.render(cmd_buffer);
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
	mesh.create_from_file("test/metallic_roughness_test/scene.gltf");

	ObjectManager::get_instance().add_mesh(mesh, "mesh", { .position = { 0,0,0 }, .rotation = {0,0,0}, .scale = { 0.1f, 0.1f, 0.1f } });


	//for (int x = -15; x < 15; x++)
	//for (int z = -15; z < 15; z++)
	////for (int z = -15; z < 15; z++)
	//{
	//	float spacing = 5.0f;
	//	Transform t = { .position = spacing * glm::vec3(x, 0, z), .rotation = {0,0,0}, .scale = glm::vec3{ 1.0f, 1.0f, 1.0f } };

	//	if ((x + z + 0) % 2 == 0)
	//	{
	//		ObjectManager::get_instance().add_mesh_instance("cube", ObjectManager::GPUInstanceData{ .model = glm::mat4(t), .color = glm::vec4(glm::sphericalRand(1.0f), 1.0f) });
	//	}
	//	else
	//	{
	//		ObjectManager::get_instance().add_mesh_instance("sphere", ObjectManager::GPUInstanceData{ .model = glm::mat4(t), .color = glm::vec4(glm::sphericalRand(1.0f), 1.0f)});
	//	}
	//}
}

void SampleProject::update_scene(float t, float dt)
{
	ObjectManager& object_manager = ObjectManager::get_instance();
	
	{
		std::string mesh_name = "mesh";
		size_t mesh_idx = object_manager.m_mesh_id_from_name.at(mesh_name);

		object_manager.m_mesh_instance_data[mesh_idx][0].model = glm::rotate(object_manager.m_mesh_instance_data[mesh_idx][0].model, dt , glm::vec3(0,1,0));
		object_manager.update_instances_ssbo(mesh_name);

		//for (size_t instance_idx = 0; instance_idx < object_manager.m_mesh_instance_data[mesh_idx].size(); instance_idx++)
		{
			//float freq = t;
			//float instance_id_norm = instance_idx / (float)object_manager.m_mesh_instance_data[mesh_idx].size();
			//object_manager.m_mesh_instance_data[mesh_idx][instance_idx].model[3].y += cos(freq * 10 * instance_id_norm) * 0.1f;
			//object_manager.m_mesh_instance_data[mesh_idx][instance_idx].model[3].z += sin(freq * 10 * instance_id_norm) * 0.1f;
		}
	}
}

void SampleProject::exit()
{
	m_gui.exit();
}

void SampleProject::on_window_resize()
{
	Application::on_window_resize();
	m_gui.on_window_resize();
	forward_renderer.on_window_resize();
	debug_line_renderer.on_window_resize();
	m_camera.update_aspect_ratio(m_window->GetWidth() / (float)m_window->GetHeight());
}

void SampleProject::on_lmb_down(MouseEvent event)
{
	m_camera.controller.b_first_click = true;
    m_camera.controller.m_last_mouse_pos = { (float)event.px, (float)event.py };
}

void SampleProject::on_mouse_move(MouseEvent event)
{
	Application::on_mouse_move(event);
	
	if (!m_gui.is_active())
	{
		if (event.b_lmb_click)
		{
			m_camera.rotate(m_delta_time, m_event_manager->mouse_event);
		}
	}

}

void SampleProject::on_key_event(KeyEvent event)
{
	Application::on_key_event(event);
	m_camera.translate(m_delta_time, m_event_manager->key_event);
}

