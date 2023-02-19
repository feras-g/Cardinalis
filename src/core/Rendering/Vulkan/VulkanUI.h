#pragma once
class VulkanUI
{
public:
	VulkanUI() = default;

	VulkanUI& Start();
	VulkanUI& ShowSceneViewportPanel(unsigned int texColorId, unsigned int texDepthId);
	VulkanUI& ShowMenuBar();
	VulkanUI& AddHierarchyPanel();
	VulkanUI& AddInspectorPanel();
	VulkanUI& AddOverlay(const char* title, float deltaSeconds);
	VulkanUI& ShowPlot(float* values, size_t nbValues);
	void End();

	bool bIsSceneViewportHovered = false;
	float fSceneViewAspectRatio = 1.0f;
};

