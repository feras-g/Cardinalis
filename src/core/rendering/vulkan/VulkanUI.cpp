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

void VulkanGUI::show_gizmo(const Camera& camera, const KeyEvent& event, glm::mat4& selected_object_transform)
{
	ImGuizmo::OPERATION current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE current_gizmo_mode = ImGuizmo::MODE::WORLD;

	ImGuizmo::BeginFrame();
	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	ImGuizmo::SetRect(0, 0, m_renderer.get_render_area().x, m_renderer.get_render_area().y);

	ImGuizmo::SetOrthographic(false);

	const glm::f32* view = glm::value_ptr(camera.get_view());
	const glm::f32* proj = glm::value_ptr(camera.get_proj());

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

	ImGuizmo::Manipulate(view, proj, gizmo_operation, ImGuizmo::MODE::WORLD, glm::value_ptr(selected_object_transform));
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

VulkanGUI& VulkanGUI::ShowSceneViewportPanel(Camera& scene_camera,
	VkDescriptorSet_T* texDeferred, VkDescriptorSet_T* texColorId,
	VkDescriptorSet_T* texNormalId, VkDescriptorSet_T* texDepthId,
	VkDescriptorSet_T* texNormalMapId, VkDescriptorSet_T* texMetallicRoughnessId)
{
	static ImGuiComboFlags flags = 0;

	const char* items[] = { "Deferred Output", "Albedo", "Normal", "Depth", "Metallic/Roughness", "Directional Shadow Map" };
	static int item_current_idx = 0; // Here we store our selection data as an index.
	const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)

	if (ImGui::Begin("Settings", 0, 0))
	{
		if (ImGui::BeginCombo("View Mode", combo_preview_value, flags))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n], is_selected))
					item_current_idx = n;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	if (ImGui::Begin("Scene", 0, 0))
	{
		is_scene_viewport_hovered = ImGui::IsWindowHovered();
		ImVec2 sceneViewPanelSize = ImGui::GetContentRegionAvail();
		const float curr_aspect_ratio = sceneViewPanelSize.x / sceneViewPanelSize.y;

		//if (curr_aspect_ratio != default_scene_view_aspect_ratio)
		//{
		//	default_scene_view_aspect_ratio = curr_aspect_ratio;
		//	scene_camera.update_aspect_ratio(default_scene_view_aspect_ratio);
		//}
		
		ImGui::Image(reinterpret_cast<ImTextureID>(texDeferred), sceneViewPanelSize);
	}
	ImGui::PopStyleVar();

	ImGui::End();

	if (ImGui::Begin("GBuffers", 0, 0))
	{
		ImVec2 thumbnail_size{ 256, 256 };
		ImGui::SetWindowSize({ thumbnail_size.x * 5, thumbnail_size.y });
		
		ImGui::Image((ImTextureID)(texColorId), thumbnail_size);
		ImGui::Image((ImTextureID)(texNormalId), thumbnail_size);
		ImGui::Image((ImTextureID)(texDepthId), thumbnail_size);
		ImGui::Image((ImTextureID)(texMetallicRoughnessId), thumbnail_size);
	}
	ImGui::End();

	return *this;
}

VulkanGUI& VulkanGUI::ShowMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	return *this;
}

VulkanGUI& VulkanGUI::AddHierarchyPanel()
{
	// Hierachy panel
	if (ImGui::Begin("Hierarchy", 0, 0))
	{

	}
	ImGui::End();

	return *this;
}

VulkanGUI& VulkanGUI::AddInspectorPanel()
{
	// Inspector panel
	if (ImGui::Begin("Inspector", 0, 0))
	{

	}
	ImGui::End();

	return *this;
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

VulkanGUI& VulkanGUI::ShowFrameTimeGraph(float* values, size_t nbValues)
{
	const float width = ImGui::GetWindowWidth();

	if (ImGui::Begin("Statistics", 0, 0))
	{
	}
	ImGui::End();

	return *this;
}

void VulkanGUI::show_camera_settings(Camera& camera)
{
	if (ImGui::Begin("Camera", 0, 0))
	{
		ImGui::SeparatorText("Transform");
		{
			
			ImGui::InputFloat3("Position", glm::value_ptr(camera.controller.m_position));
		}

		ImGui::SeparatorText("Camera");
		{
			ImGui::Text("Aspect Ratio: %f", camera.aspect_ratio);

			if (ImGui::SliderFloat("Fov", &camera.fov, 1.0f, 120.0f))
			{
				camera.update_proj();
			}

			if (ImGui::SliderFloat("Near", &camera.znear, 0.1f, 1000.0f, "%.2f"))
			{
				camera.update_proj();
			}

			if(ImGui::SliderFloat("Far", &camera.zfar, 0.0f, 1000.0f, "%.2f"))
			{
				camera.update_proj();
			}

			Camera::ProjectionMode& mode = camera.projection_mode;

			if (ImGui::RadioButton("Perspective", mode == Camera::ProjectionMode::PERSPECTIVE))  
			{
				mode = Camera::ProjectionMode::PERSPECTIVE;
				camera.update_proj();
			}

			ImGui::SameLine();
			if (ImGui::RadioButton("Orthographic", mode == Camera::ProjectionMode::ORTHOGRAPHIC))
			{ 
				mode = Camera::ProjectionMode::ORTHOGRAPHIC;
			} 

			if (mode == Camera::ProjectionMode::ORTHOGRAPHIC)
			{
				float& left = camera.left;
				float& right = camera.right;
				float& top = camera.top;
				float& bottom = camera.bottom;

				float planes[4] = { left, right, top, bottom };

				ImGui::SliderFloat4("Orthographic Planes", planes, -10.0f, 10.0f);

				camera.update_proj();
			}

		}
	}

	ImGui::End();
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

bool VulkanGUI::is_scene_viewport_active()
{
	return m_is_scene_viewport_hovered;
}

void VulkanGUI::show_viewport_window(Camera& camera)
{
	if (ImGui::Begin("Scene Viewport"))
	{
		scene_viewport_window_size = ImGui::GetContentRegionAvail();
		
		const float curr_aspect_ratio = scene_viewport_window_size.x / scene_viewport_window_size.y;

		if (curr_aspect_ratio != scene_view_aspect_ratio)
		{
			scene_view_aspect_ratio = curr_aspect_ratio;
			camera.update_aspect_ratio(scene_view_aspect_ratio);
		}

		m_is_scene_viewport_hovered = ImGui::IsItemActive();
		ImGui::Image(m_renderer.scene_viewport_attachments_ids[context.curr_frame_idx], scene_viewport_window_size);
	}

	ImGui::End();
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

VulkanGUI& VulkanGUI::ShowInspector()
{
	if (ImGui::Begin("Inspector", 0, 0))
	{
		if (ImGui::TreeNode("Hierarchy"))
		{

		}
	}
	ImGui::End();

	return *this;
}

VulkanGUI& VulkanGUI::ShowLightSettings(LightManager* light_manager)
{
	static float radius = 1.0f;
	static glm::vec3 offset = {};

	if (ImGui::Begin("Lights", 0, 0))
	{
		/* Directional Light */
		ImGui::SeparatorText("Directional Light");
		ImGui::DragFloat4("Direction", glm::value_ptr(light_manager->light_data.directional_light.direction), 0.01f);
		ImGui::DragFloat4("Color",	   glm::value_ptr(light_manager->light_data.directional_light.color), 0.1f);
		ImGui::DragFloat("Cycle Speed", &light_manager->cycle_speed);
		ImGui::SameLine();
		ImGui::Checkbox("Cycle", &light_manager->b_cycle_directional_light);
		
		/* Point Lights */
		ImGui::SeparatorText("Point Lights");
		if (ImGui::Button("Add Point Light"))
		{
			light_manager->light_data.point_lights.push_back({});
			light_manager->light_data.num_point_lights++;
		}

		if (ImGui::DragFloat("Global Radius", &radius, 0.01f))
		{
			for (unsigned i = 0; i < light_manager->light_data.num_point_lights; i++)
			{
				light_manager->light_data.point_lights[i].radius = radius;
			}
			light_manager->update_ssbo();
		}

		if (ImGui::DragFloat3("Global Offset", glm::value_ptr(offset), 0.01f))
		{
			for (unsigned i = 0; i < light_manager->light_data.num_point_lights; i++)
			{
				light_manager->light_data.point_lights[i].position.x += offset.x;
				light_manager->light_data.point_lights[i].position.y += offset.y;
				light_manager->light_data.point_lights[i].position.z += offset.z;
			}

			light_manager->update_ssbo();
			offset = {};
		}

		ImGui::Checkbox("Animate", &light_manager->b_animate_point_lights);
		ImGui::SameLine();
		ImGui::DragFloat("Animate Speed", &light_manager->anim_freq, 0.01f);

	}

	ImGui::End();

	return *this;
}

