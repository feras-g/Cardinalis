#include "Rendering/Vulkan/VulkanUI.h"

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

VulkanUI& VulkanUI::ShowSceneViewportPanel(unsigned int texColorId, unsigned int texDepthId)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	if (ImGui::Begin("Scene", 0, 0))
	{
		ImVec2 sceneViewPanelSize = ImGui::GetContentRegionAvail();
		const float currAspect = sceneViewPanelSize.x / sceneViewPanelSize.y;

		if (currAspect != fSceneViewAspectRatio)
		{
			fSceneViewAspectRatio = currAspect;
		}

		if (texColorId != 0)
		{
			ImGui::Image((ImTextureID)texColorId, sceneViewPanelSize);
		}
		bIsSceneViewportHovered = ImGui::IsItemHovered();
	}

	
	ImGui::End();


	ImGui::PopStyleVar();

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

VulkanUI& VulkanUI::AddOverlay(const char* title, float deltaSeconds)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGuiWindowFlags overlayFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
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
		ImGui::Text("CPU : %.2f ms (%.1f fps)", deltaSeconds * 1000.0f, 1.0f / deltaSeconds);
	}

	ImGui::End();

	return *this;
}

VulkanUI& VulkanUI::ShowPlot(float* values, size_t nbValues)
{
	ImVec2 plotextent(ImGui::GetContentRegionAvail().x, 250);
	ImGui::Text("I am a fancy tooltip");
	ImGui::PlotHistogram("Lines", values, nbValues, 0, 0, 0.0f, 32.0f, plotextent);
	return *this;
}
