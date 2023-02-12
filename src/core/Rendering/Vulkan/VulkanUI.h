#pragma once
class VulkanUI
{
public:
	VulkanUI() = default;

	VulkanUI& Start();
	VulkanUI& ShowSceneViewportPanel(unsigned int texColorId, unsigned int texDepthId);
	VulkanUI& AddHierarchyPanel();
	VulkanUI& AddInspectorPanel();
	VulkanUI& AddOverlay(const char* title, float deltaSeconds);
	void End();

	bool bIsSceneViewportHovered = false;
	float fSceneViewAspectRatio = 1.0f;
};

