#include "core/engine/InputEvents.h"
#include "core/rendering/Camera.h"
#include "core/rendering/FrameCounter.h"
#include "core/rendering/vulkan/RenderObjectManager.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/VulkanUI.h"

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

void VulkanGUI::show_toolbar()
{

}

static Primitive* selected_primitive = nullptr;
static VulkanMesh* selected_mesh = nullptr;

void VulkanGUI::show_hierarchy(ObjectManager& object_manager)
{
	ImGui::ShowDemoWindow();
	if (ImGui::Begin("Inspector"))
	{
		for (size_t i = 0; i < object_manager.m_meshes.size(); i++)
		{
			if (ImGui::TreeNode(object_manager.m_mesh_names[i].c_str()))
			{
				VulkanMesh& mesh = object_manager.m_meshes[i];
				//selected_object_transform = &mesh.model;
				for (size_t prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
				{
					Primitive& prim = mesh.geometry_data.primitives[prim_idx];

					if (ImGui::TreeNode((void*)(intptr_t)prim_idx, prim.name.c_str(), prim_idx))
					{
						selected_mesh = &mesh;
						selected_primitive = &prim;

						// WIP
						ImGui::Text("Material : %s (id %i)", object_manager.m_material_names[prim.material_id].c_str(), prim.material_id);
						ImGui::Text("World Center : %f %f %f", prim.world_center.x, prim.world_center.y, prim.world_center.z);


						for (int i = 0; i < 4; i++)
						{
							ImGui::Text("%f %f %f %f", selected_primitive->model_world_center[i].x, selected_primitive->model_world_center[i].y, selected_primitive->model_world_center[i].z, selected_primitive->model_world_center[i].w);
						}
						ImGui::Text("");
						for (int i = 0; i < 4; i++)
						{
							ImGui::Text("%f %f %f %f", selected_primitive->model[i].x, selected_primitive->model[i].y, selected_primitive->model[i].z, selected_primitive->model[i].w);
						}
						ImGui::Text("");
						for (int i = 0; i < 4; i++)
						{
							ImGui::Text("%f %f %f %f", selected_primitive->offset[i].x, selected_primitive->offset[i].y, selected_primitive->offset[i].z, selected_primitive->offset[i].w);
						}




						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
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

void VulkanGUI::show_draw_metrics()
{
	start_overlay("Draw statistics");

	ImGui::Text("DRAW STATISTICS");
	for (size_t i = 0; i < DrawMetricsManager::renderer_names.size(); i++)
	{
		ImGui::Text("%s", DrawMetricsManager::renderer_names[i]);
		ImGui::Indent();
		ImGui::BulletText("Draw calls : %u", DrawMetricsManager::num_drawcalls[i]);
		ImGui::BulletText("Num Vertices : %u", DrawMetricsManager::num_vertices[i]);
		ImGui::BulletText("Num Instances : %u", DrawMetricsManager::num_instances[i]);

		ImGui::Unindent();
	}

	ImGui::Text("Total:");
	ImGui::BulletText("Draw calls : %u", DrawMetricsManager::total_drawcalls);
	ImGui::BulletText("Num Vertices : %u", DrawMetricsManager::total_vertices);
	ImGui::BulletText("Num Instances : %u", DrawMetricsManager::total_instances);

	ImGui::End();
}

bool VulkanGUI::is_inactive()
{
	return !ImGui::IsAnyItemActive();
}

bool VulkanGUI::is_not_selecting_gizmo() const
{
	return !ImGuizmo::IsUsing();
}

bool VulkanGUI::is_scene_viewport_active() const
{
	return b_is_scene_viewport_active;
}

void VulkanGUI::show_viewport_window(ImTextureID scene_image_id, camera& camera, ObjectManager& object_manager)
{
	static ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	static ImGuizmo::MODE transform_mode = ImGuizmo::MODE::WORLD;

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

		if (ImGui::Button("Local"))
		{
			transform_mode = ImGuizmo::MODE::LOCAL;
		}
		if (ImGui::Button("World"))
		{
			transform_mode = ImGuizmo::MODE::WORLD;
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



	if (ImGui::Begin("Scene"))
	{
		if (viewport_aspect_ratio != camera.aspect_ratio)
		{
			camera.update_aspect_ratio(viewport_aspect_ratio);
		}

		b_is_scene_viewport_active = ImGui::IsItemActive();

		/* Scene view */
		ImVec2 window_size = ImGui::GetContentRegionAvail();
		ImGui::Image(scene_image_id, window_size);
		viewport_aspect_ratio = window_size.x / window_size.y;

		/* Gizmos */
		ImGuizmo::BeginFrame();
		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window_size.x, window_size.y);
		ImGuizmo::SetOrthographic(false);

		glm::mat4 gizmo_center = glm::identity<glm::mat4>();

		//if (object_manager.m_mesh_instance_data[object_manager.current_selected_mesh_id].size() > 0)
		{
			const glm::f32* view = glm::value_ptr(camera.view);
			const glm::f32* proj = glm::value_ptr(camera.projection);

			if (selected_primitive != nullptr)
			{
				glm::mat4 orig = selected_primitive->model_world_center;

				if (ImGuizmo::Manipulate(view, proj, gizmo_operation, transform_mode, glm::value_ptr(selected_primitive->model_world_center)))
				{
					selected_primitive->offset += selected_primitive->model_world_center - orig;
				}

				selected_primitive->model[3].x += selected_primitive->offset[3].x;
				selected_primitive->model[3].y += selected_primitive->offset[3].y;
				selected_primitive->model[3].z += selected_primitive->offset[3].z;

				selected_primitive->offset = glm::mat4(0);
			}
		}

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

