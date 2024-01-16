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
#include "rendering/vulkan/Renderers/ShadowRenderer.hpp"
#include "rendering/vulkan/Renderers/VolumetricLightRenderer.hpp"

#include "rendering/lighting.h"

static light_manager lights;
static ForwardRenderer forward_renderer;
static DebugLineRenderer debug_line_renderer;
static DeferredRenderer deferred_renderer;
static SkyboxRenderer skybox_renderer;
static IBLRenderer ibl_renderer;
static ShadowRenderer shadow_renderer;
static VolumetricLightRenderer volumetric_light_renderer;
static std::vector<size_t> drawable_list;

SampleProject::SampleProject(const char* title, uint32_t width, uint32_t height)
	: Application(title, width, height)
{
	
}

void SampleProject::init()
{
	ObjectManager::get_instance().init();

	lights.init();
	shadow_renderer.init();
	debug_line_renderer.init();
	forward_renderer.p_debug_line_renderer = &debug_line_renderer;
	forward_renderer.init();

	volumetric_light_renderer.create_renderpass();
	volumetric_light_renderer.is_initialized = true;
	ibl_renderer.init("pisa.hdr");
	deferred_renderer.init();
	volumetric_light_renderer.create_pipeline();

	skybox_renderer.init();
	m_camera.update_aspect_ratio(1.0f);
	skybox_renderer.init(cubemap_renderer.cubemap_attachment);
	create_scene();
	lights.write_ssbo();
}

void SampleProject::compose_gui()
{
	ObjectManager& object_manager = ObjectManager::get_instance();
	m_gui.begin();
	m_gui.show_toolbar();
	m_gui.show_hierarchy(object_manager);
	//m_gui.show_draw_metrics();
	m_gui.show_shader_library();
	m_gui.show_viewport_window(deferred_renderer.ui_texture_ids[ctx.curr_frame_idx].light_accumulation, m_camera, object_manager);
	m_camera.show_ui();
	deferred_renderer.show_ui(m_camera);
	ibl_renderer.show_ui();
	shadow_renderer.show_ui();
	lights.show_ui();
	volumetric_light_renderer.show_ui();
	m_gui.end();
}

void SampleProject::update_frame_ubo()
{
	VulkanRendererCommon::FrameData& data = VulkanRendererCommon::get_instance().m_framedata[ctx.curr_frame_idx];
	data.view = m_camera.view;
	data.proj = m_camera.projection;
	data.view_proj = data.proj* data.view;
	data.view_proj_inv = glm::inverse(data.view_proj);
	data.camera_pos_ws = glm::vec4(m_camera.position, 1);
	data.time = m_time;
	VulkanRendererCommon::get_instance().update_frame_data(data, ctx.curr_frame_idx);
}

void SampleProject::update(float t, float dt)
{
	/* Update camera */
	if (m_window->is_in_focus())
	{
		//if (m_gui.is_inactive())
		{
			if (m_event_manager->key_event.is_key_pressed_async(Key::Z))
			{
				m_camera.translate(m_camera.forward * m_delta_time);
			}

			if (m_event_manager->key_event.is_key_pressed_async(Key::S))
			{
				m_camera.translate(-m_camera.forward * m_delta_time);
			}

			if (m_event_manager->key_event.is_key_pressed_async(Key::Q))
			{
				m_camera.right = glm::normalize(glm::cross(m_camera.forward, m_camera.up));
				m_camera.translate(-m_camera.right * m_delta_time);
			}

			if (m_event_manager->key_event.is_key_pressed_async(Key::D))
			{
				m_camera.right = glm::normalize(glm::cross(m_camera.forward, m_camera.up));
				m_camera.translate(m_camera.right * m_delta_time);
			}

			if (m_event_manager->key_event.is_key_pressed_async(Key::SPACE))
			{
				m_camera.translate(m_camera.up * m_delta_time);
			}

			if (m_event_manager->key_event.is_key_pressed_async(Key::LSHIFT))
			{
				m_camera.translate(-m_camera.up * m_delta_time);
			}
		}
	}

	compose_gui();
}

void SampleProject::render()
{
	VkCommandBuffer& cmd_buffer = ctx.get_current_frame().cmd_buffer;
	DrawMetricsManager::reset();

	set_polygon_mode(cmd_buffer, IRenderer::global_polygon_mode);

	ctx.swapchain->clear_color(cmd_buffer);
	update_gpu_buffers();

	shadow_renderer.render(cmd_buffer, drawable_list, m_camera, VulkanRendererCommon::get_instance().m_framedata[ctx.curr_frame_idx], lights.dir_light.dir);

	deferred_renderer.render_geometry_pass(cmd_buffer, drawable_list);

	volumetric_light_renderer.render(cmd_buffer);	// Volumetric renderer needs depth buffer written by geometry pass

	deferred_renderer.render_lighting_pass(cmd_buffer);

	skybox_renderer.render(cmd_buffer);
	m_gui.render(cmd_buffer);
}

void SampleProject::update_gpu_buffers()
{
	update_frame_ubo();
	update_instances_ssbo();
}

void SampleProject::create_scene()
{
	//VulkanMesh plane;
	//plane.create_from_file("basic/unit_plane.glb");
	//drawable_list.push_back(ObjectManager::get_instance().add_mesh(plane, "Floor", { .position = { 0,0,0 }, .rotation = {0,0,0}, .scale = {  25, 25, 25 } }));

	VulkanMesh sponza;
	sponza.create_from_file("scenes/sponza/scene.gltf");
	drawable_list.push_back(ObjectManager::get_instance().add_mesh(sponza, "Sponza", { .position = { 0,0,0 }, .rotation = {0,0,0}, .scale = {  1, 1, 1 } }, true));
}

void SampleProject::update_instances_ssbo()
{
	/* Update CPU scene data */
	static ObjectManager& object_manager = ObjectManager::get_instance();

	//for (size_t i = 0; i < object_manager.m_meshes.size(); i++)
	//{
	//	object_manager.update_instances_ssbo(object_manager.m_mesh_names[i]);
	//}
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
	skybox_renderer.on_window_resize();
	deferred_renderer.on_window_resize();
}

void SampleProject::on_mouse_move(MouseEvent event)
{
	Application::on_mouse_move(event);

	//if (m_gui.is_inactive())
	{
		glm::vec2 a(event.curr_click_px, event.curr_click_py);
		if (m_event_manager->mouse_event.b_mmb_click)
		{
			glm::vec2 b = { event.curr_pos_x,event.curr_pos_y };

			glm::vec3 dir = glm::vec3(glm::normalize(b - a), 0);

			glm::vec3 t = { dir.x, -dir.y, 0 };
			m_camera.translate(t * m_delta_time);

			a = b;
		}


		if (m_event_manager->mouse_event.b_lmb_click && m_gui.is_not_selecting_gizmo() && m_gui.is_scene_viewport_active())
		{
			glm::vec2 mouse_pos = { m_event_manager->mouse_event.curr_pos_x, m_event_manager->mouse_event.curr_pos_y };
			m_camera.rotate(mouse_pos);
			m_camera.update_view();
		}
	}

}

void SampleProject::on_key_event(KeyEvent event)
{
	Application::on_key_event(event);
}

void SampleProject::on_mouse_up(MouseEvent event)
{
	Application::on_mouse_up(event);
}

void SampleProject::on_mouse_down(MouseEvent event)
{
	Application::on_mouse_down(event);

	//if (m_gui.is_inactive())
	{
		if (event.b_lmb_click)
		{
			m_camera.first_mouse_click = true;
		}
	}

}