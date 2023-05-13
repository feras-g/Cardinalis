#include "Rendering/Vulkan/VulkanUI.h"
#include "Rendering/FrameCounter.h"
#include "Rendering/Camera.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

VulkanUI& VulkanUI::Start()
{
	ImGui::NewFrame();
	ImGui::StyleColorsDark();
	// Global user interface consisting of multiple viewports
	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

	return *this;
}


void VulkanUI::End()
{
	ImGui::Render();
}

VulkanUI& VulkanUI::ShowSceneViewportPanel(
	size_t texDeferred, size_t texColorId,
	size_t texNormalId, size_t texDepthId,
	size_t texNormalMapId, size_t texMetallicRoughnessId, size_t texShadowMapId
)
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
	if (ImGui::Begin("Scene", 0, ImGuiWindowFlags_MenuBar |  ImGuiWindowFlags_NoDecoration))
	{
		ImVec2 sceneViewPanelSize = ImGui::GetContentRegionAvail();
		const float currAspect = sceneViewPanelSize.x / sceneViewPanelSize.y;

		if (currAspect != fSceneViewAspectRatio)
		{
			fSceneViewAspectRatio = currAspect;
		}
		

		//if (combo_preview_value == "Deferred Output")
		{
			ImGui::Image(reinterpret_cast<ImTextureID>(texDeferred), sceneViewPanelSize );
		}


		bIsSceneViewportHovered = ImGui::IsItemHovered();
	}
	ImGui::PopStyleVar();

	ImGui::End();


	if (ImGui::Begin("GBuffers", 0, 0))
	{
		ImVec2 thumbnail_size{ 512, 512 };
		//ImGui::SetWindowSize({ thumbnail_size.x * 5, thumbnail_size.y });
		
		{
			ImGui::Text("Albedo");
			ImGui::Image(reinterpret_cast<ImTextureID>(texColorId), thumbnail_size);
		}

		{
			ImGui::Text("WS Normal");
			ImGui::Image(reinterpret_cast<ImTextureID>(texNormalId), thumbnail_size);
		}

		{
			ImGui::Text("Depth");
			ImGui::Image(reinterpret_cast<ImTextureID>(texDepthId), thumbnail_size);

		}

		{
			ImGui::Text("Metallic/Roughness");
			ImGui::Image(reinterpret_cast<ImTextureID>(texMetallicRoughnessId), thumbnail_size);

		}

		{
			ImGui::Text("Depth (Directional Light)");
			ImGui::Image(reinterpret_cast<ImTextureID>(texShadowMapId), thumbnail_size);
		}
		
	}
	ImGui::End();





	return *this;
}

VulkanUI& VulkanUI::ShowMenuBar()
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

VulkanUI& VulkanUI::AddHierarchyPanel()
{
	// Hierachy panel
	if (ImGui::Begin("Hierarchy", 0, 0))
	{

	}
	ImGui::End();

	return *this;
}

VulkanUI& VulkanUI::AddInspectorPanel()
{
	// Inspector panel
	if (ImGui::Begin("Inspector", 0, 0))
	{

	}
	ImGui::End();

	return *this;
}

VulkanUI& VulkanUI::ShowStatistics(const char* title, float cpuDeltaSecs, size_t frameNumber)
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

VulkanUI& VulkanUI::ShowFrameTimeGraph(float* values, size_t nbValues)
{
	const float width = ImGui::GetWindowWidth();

	if (ImGui::Begin("Statistics", 0, 0))
	{
		//ImGui::PlotLines("CPU Time", values, nbValues, 0, 0, 0, 33);
	}
	ImGui::End();

	return *this;
}

VulkanUI& VulkanUI::ShowCameraSettings(Camera* camera)
{
	if (ImGui::Begin("Camera", 0, 0))
	{
		ImGui::SetNextItemOpen(true);
		if (ImGui::TreeNode("Controller Settings"))
		{
			static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
			static int vec4i[4] = { 1, 5, 100, 255 };

			ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
			
			ImGui::SeparatorText("Transform");
			ImGui::DragFloat3("Location", glm::value_ptr(camera->controller.m_position), 0.1f);
			ImGui::DragFloat3("Rotation", glm::value_ptr(camera->controller.m_rotation), 0.1f);

			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(true);
		if (ImGui::TreeNode("Camera Settings"))
		{
			ImGui::InputFloat("FOV", &camera->fov);
			ImGui::InputFloat2("Near-Far", glm::value_ptr(camera->near_far));

			ImGui::TreePop();
		}
	}

	ImGui::End();

	return *this;
}

VulkanUI& VulkanUI::ShowInspector()
{
	if (ImGui::Begin("Inspector", 0, 0))
	{
		if (ImGui::TreeNode("Hierarchy"))
		{
			for (int i = 0; i < RenderObjectManager::drawables.size(); i++)
			{
				// Use SetNextItemOpen() so set the default state of a node to be open. We could
				// also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
				if (i == 0)
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);

				if (ImGui::TreeNode((void*)(intptr_t)i, RenderObjectManager::drawable_names[i].c_str(), i))
				{
					//ImGui::DragFloat3("Translation", glm::value_ptr(RenderObjectManager::transform_datas[i].translation), 0.1f);
					//ImGui::SliderFloat4("Rotation",  glm::value_ptr(RenderObjectManager::transform_datas[i].rotation), 0.0f, 1.0f);
					//ImGui::DragFloat3("Scale", glm::value_ptr(RenderObjectManager::transform_datas[i].scale), 0.1f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();


	return *this;
}

VulkanUI& VulkanUI::ShowLightSettings(LightManager* light_manager)
{
	if (ImGui::Begin("Lights", 0, 0))
	{
		ImGui::SeparatorText("Directional Light");
		
		ImGui::DragFloat3("Direction", glm::value_ptr(light_manager->m_light_data.directional_light.direction), 0.01f);
		ImGui::DragFloat3("Eye", glm::value_ptr(light_manager->eye), 0.01f);
		ImGui::DragFloat3("Up", glm::value_ptr(light_manager->up), 0.01f);
		ImGui::DragFloat3("Bbox min", glm::value_ptr(light_manager->view_volume_bbox_min), 0.1f);
		ImGui::DragFloat3("Bbox max", glm::value_ptr(light_manager->view_volume_bbox_max), 0.1f);
		ImGui::DragFloat3("Color",	  glm::value_ptr(light_manager->m_light_data.directional_light.color), 0.1f);
	}

	ImGui::End();

	return *this;
}