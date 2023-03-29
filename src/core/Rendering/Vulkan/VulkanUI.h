#pragma once

struct Camera;

class VulkanUI
{
public:
	VulkanUI() = default;

	VulkanUI& Start();
	VulkanUI& ShowSceneViewportPanel(unsigned int texColorId, unsigned int texDepthId);
	VulkanUI& ShowMenuBar();
	VulkanUI& AddHierarchyPanel();
	VulkanUI& AddInspectorPanel();
	VulkanUI& ShowStatistics(const char* title, float cpuDeltaSecs, size_t frameNumber);
	VulkanUI& ShowFrameTimeGraph(float* values, size_t nbValues);
	VulkanUI& ShowCameraSettings(Camera* camera);
	void End();

	bool bIsSceneViewportHovered = false;
	float fSceneViewAspectRatio = 1.0f;
};

