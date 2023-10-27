#include "core/rendering/vulkan/VulkanUI.h"
#include "core/rendering/FrameCounter.h"
#include "core/rendering/Camera.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/Renderers/DeferredRenderer.h"
#include "core/rendering/vulkan/Renderers/ShadowRenderer.h"
#include "core/engine/InputEvents.h"
#include "core/rendering/vulkan/RenderObjectManager.h"


#include "../imgui/imgui.h"
#include "../imgui/widgets/imguizmo/ImGuizmo.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

#include "glm/gtx/quaternion.hpp"

VulkanGUI::VulkanGUI()
{
	m_renderer.init();
}

void VulkanGUI::begin()
{
	ImGui::NewFrame();
	ImGui::StyleColorsDark();
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

	const glm::f32* view   = glm::value_ptr(camera.get_view());
	const glm::f32* proj   = glm::value_ptr(camera.get_proj());

	glm::mat4 transform = glm::identity<glm::mat4>();
	ImGuizmo::DrawGrid(view, proj, glm::value_ptr(transform), 10.0f);

	static ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

	if (event.is_key_pressed_async(Key::R))
	{
		gizmo_operation = ImGuizmo::OPERATION::ROTATE;
	}
	else if (event.is_key_pressed_async(Key::T))
	{
		gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	}

	ImGuizmo::Manipulate(view, proj, gizmo_operation, ImGuizmo::MODE::WORLD, glm::value_ptr(selected_object_transform));
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

bool VulkanGUI::is_active()
{
	return ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();
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

		if (curr_aspect_ratio != default_scene_view_aspect_ratio)
		{
			default_scene_view_aspect_ratio = curr_aspect_ratio;
			scene_camera.update_aspect_ratio(default_scene_view_aspect_ratio);
		}
		
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

VulkanGUI& VulkanGUI::show_overlay(const char* title, float cpuDeltaSecs, size_t frameNumber)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGuiWindowFlags overlayFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
		 ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;

	ImVec2 work_pos =  viewport->WorkPos;
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_pos.y + work_size.y - PAD);

	window_pos_pivot.x = 0.0f;
	window_pos_pivot.y = 1.0f;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(0.33f); // Transparent background
	if (ImGui::Begin("Overlay", 0, overlayFlags))
	{
		ImGui::Text(title);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Frame: %d", frameNumber);
		ImGui::Text("Avg CPU Frame-Time (ms): %.2f", FrameStats::CurrentFrameTimeAvg);
	}

	ImGui::End();

	return *this;
}

VulkanGUI& VulkanGUI::ShowFrameTimeGraph(float* values, size_t nbValues)
{
	const float width = ImGui::GetWindowWidth();

	if (ImGui::Begin("Statistics", 0, 0))
	{
		//ImGui::PlotLines("CPU Time", values, nbValues, 0, 0, 0, 33);
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
			if (ImGui::DragFloat3("Position", glm::value_ptr(camera.controller.m_position), 0.1f))
			{
				camera.controller.update_view();
			}
		}

		ImGui::SeparatorText("Camera");
		{
			if (ImGui::SliderFloat("Fov", &camera.fov, 1.0f, 120.0f))
			{
				camera.update_proj();
			}

			float& znear = camera.znear;
			float& zfar  = camera.zfar;
			float near_far[2] = { znear, zfar };
			ImGui::DragFloat2("Near/Far", near_far, 0.1f);

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

VulkanGUI& VulkanGUI::ShowShadowPanel(CascadedShadowRenderer* shadow_renderer, std::span<VkDescriptorSet> texShadowCascades)
{
	if (ImGui::Begin("Shadow Settings", 0, 0))
	{
		ImGui::SeparatorText("Cascaded Shadows");

		if (ImGui::DragFloat("Z-Split Interpolation Factor", &shadow_renderer->lambda, 0.01f))
		{
			shadow_renderer->compute_cascade_splits();
		}

		/* Display shadow cascades */
		//for (int i = 0; i < texShadowCascades.size(); i++)
		//{
		//	ImGui::Image(reinterpret_cast<ImTextureID>(texShadowCascades[i]), { 256, 256 });
		//	ImGui::SameLine();
		//}
	}


	ImGui::End();

	return *this;
}
