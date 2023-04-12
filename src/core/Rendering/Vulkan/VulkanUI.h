#pragma once

#include "glm/gtx/log_base.hpp"
#include <glm/gtc/type_ptr.hpp> // value_ptr

struct Camera;

class VulkanUI
{
public:
	VulkanUI() = default;

	VulkanUI& Start();
	VulkanUI& ShowSceneViewportPanel(unsigned int texDeferred, unsigned int texColorId, unsigned int texNormalId, unsigned int texDepthId);
	VulkanUI& ShowMenuBar();
	VulkanUI& AddHierarchyPanel();
	VulkanUI& AddInspectorPanel();
	VulkanUI& ShowStatistics(const char* title, float cpuDeltaSecs, size_t frameNumber);
	VulkanUI& ShowFrameTimeGraph(float* values, size_t nbValues);
	VulkanUI& ShowCameraSettings(Camera* camera);
	void End();

	bool bIsSceneViewportHovered = false;
	float fSceneViewAspectRatio = 1.0f;

	/* To remove */
	glm::vec3 pos{0,0,0};
	glm::vec3 rot{0,0,0};
	glm::vec3 scale{1,1,1};
};

