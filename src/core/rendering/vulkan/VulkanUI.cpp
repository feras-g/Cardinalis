#include "core/engine/InputEvents.h"
#include "core/rendering/Camera.h"
#include "core/rendering/FrameCounter.h"
#include "core/rendering/vulkan/LightManager.h"
#include "core/rendering/vulkan/RenderObjectManager.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/VulkanUI.h"

#include "glm/gtx/quaternion.hpp"

#include "../imgui/imgui.h"
#include "../imgui/widgets/imguizmo/ImGuizmo.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

static ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

VulkanGUI::VulkanGUI()
{
	m_renderer.init();
}

void VulkanGUI::begin()
{
	ImGui::NewFrame();
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 5.0f;
	ImGui::GetStyle().FrameRounding = 5.0f;

	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);
}

void VulkanGUI::show_demo()
{
	ImGui::ShowDemoWindow();
}

void VulkanGUI::end()
{
	ImGui::Render();
}

void VulkanGUI::exit()
{
	m_renderer.destroy();
}

void VulkanGUI::show_gizmo(const camera& camera, const KeyEvent& event, glm::mat4& selected_object_transform)
{
	ImGuizmo::OPERATION current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE current_gizmo_mode = ImGuizmo::MODE::WORLD;

	ImGuizmo::BeginFrame();
	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	ImGuizmo::SetRect(0, 0, m_renderer.get_render_area().x, m_renderer.get_render_area().y);

	ImGuizmo::SetOrthographic(false);

	//const glm::f32* view = glm::value_ptr(camera.get_view());
	//const glm::f32* proj = glm::value_ptr(camera.get_proj());

	glm::mat4 transform = glm::identity<glm::mat4>();

	const char* items[] = { "Translate", "Rotate", "Scale" };
	static int current_item = 0;

	//if (event.is_key_pressed_async(Key::R))
	//{
	//	gizmo_operation = ImGuizmo::OPERATION::ROTATE;
	//}
	//else if (event.is_key_pressed_async(Key::T))
	//{
	//	gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	//}

	//ImGuizmo::Manipulate(view, proj, gizmo_operation, ImGuizmo::MODE::WORLD, glm::value_ptr(selected_object_transform));
}

void VulkanGUI::show_toolbar()
{
	if (ImGui::Begin("Toolbar"))
	{
		/* Gizmo mode */
		ImGui::SeparatorText("Transform Mode");

		if (ImGui::Button("Translation", ImVec2(70, 40)))
		{
			gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

		}
		ImGui::SameLine();
		if (ImGui::Button("Rotation", ImVec2(70, 40)))
		{
			ImGui::SameLine();
			gizmo_operation = ImGuizmo::OPERATION::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::Button("Scale", ImVec2(70, 40)))
		{
			ImGui::SameLine();
			gizmo_operation = ImGuizmo::OPERATION::SCALE;
		}

		/* Render modes */
		ImGui::SeparatorText("Rendering mode");
		if (ImGui::Button("Default"))
		{
			IRenderer::global_polygon_mode = VK_POLYGON_MODE_FILL;
		}
		ImGui::SameLine();
		if (ImGui::Button("Wireframe"))
		{
			IRenderer::global_polygon_mode = VK_POLYGON_MODE_LINE;
		}
	}
	ImGui::End();
}

void VulkanGUI::show_inspector(const ObjectManager& object_manager)
{

}

void VulkanGUI::render(VkCommandBuffer cmd_buffer)
{
	m_renderer.render(cmd_buffer);
}

void VulkanGUI::on_window_resize()
{
	m_renderer.create_renderpass();
}

const glm::vec2& VulkanGUI::get_render_area() const
{
	return m_renderer.get_render_area();
}


void VulkanGUI::start_overlay(const char* title)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGuiWindowFlags overlayFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
		 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;
	const float PAD = 10.0f;

	ImVec2 work_pos =  viewport->WorkPos;
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_size.y - PAD);

	window_pos_pivot.x = 0.0f;
	window_pos_pivot.y = 1.0f;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(1.0f); // Transparent background

	ImGui::Begin(title, 0, overlayFlags);
}

void VulkanGUI::show_draw_statistics(IRenderer::DrawStats draw_stats)
{
	start_overlay("Draw statistics");

	ImGui::Text("DRAW STATISTICS");
	for (size_t i = 0; i < draw_stats.renderer_names.size(); i++)
	{
		ImGui::Text("%s", draw_stats.renderer_names[i]);
		ImGui::Indent();
		ImGui::BulletText("Draw calls : %u", draw_stats.num_drawcalls[i]);
		ImGui::BulletText("Num Vertices : %u", draw_stats.num_vertices[i]);
		ImGui::BulletText("Num Instances : %u", draw_stats.num_instances[i]);

		ImGui::Unindent();
	}

	ImGui::Text("Total:");
	ImGui::BulletText("Draw calls : %u", draw_stats.total_drawcalls);
	ImGui::BulletText("Num Vertices : %u", draw_stats.total_vertices);
	ImGui::BulletText("Num Instances : %u", draw_stats.total_instances);


	ImGui::End();
}

bool VulkanGUI::is_inactive()
{
	return !ImGui::IsAnyItemActive();
}

bool VulkanGUI::is_scene_viewport_hovered()
{
	return m_is_scene_viewport_hovered;
}

void VulkanGUI::show_viewport_window(camera& camera)
{
	//if (ImGui::Begin("Scene Viewport"))
	//{
	//	scene_viewport_window_size = ImGui::GetContentRegionAvail();
	//	
	//	const float curr_aspect_ratio = scene_viewport_window_size.x / scene_viewport_window_size.y;

	//	if (curr_aspect_ratio != scene_view_aspect_ratio)
	//	{
	//		scene_view_aspect_ratio = curr_aspect_ratio;
	//		camera.aspect_ratio = curr_aspect_ratio;
	//		camera.update_projection();
	//	}

	//	ImVec2 window_pos = ImGui::GetWindowPos();
	//	ImVec2 window_size = ImGui::GetWindowSize();
	//	ImVec2 cursor_pos = ImGui::GetCursorPos();

	//	m_is_scene_viewport_hovered = ImGui::IsWindowHovered();
	//	ImGui::Image(m_renderer.scene_viewport_attachments_ids[context.curr_frame_idx], scene_viewport_window_size);
	//}

	//ImGui::End();
}
void VulkanGUI::show_shader_library()
{
	if (ImGui::Begin("Shaders"))
	{
		for (const std::string& shader : Shader::shader_library.names)
		{
			ImGui::Text("%s", shader.c_str()); ImGui::SameLine();
			if (ImGui::Button("Recompile"))
			{

			}
		}
	}
	ImGui::End();
}

