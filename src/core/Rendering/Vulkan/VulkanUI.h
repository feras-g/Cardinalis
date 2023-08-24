#pragma once

#include "glm/gtx/log_base.hpp"
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <span>

class Camera;
struct LightManager;
struct CascadedShadowRenderer;
struct VkDescriptorSet_T;

class VulkanUI
{
public:
	VulkanUI() = default;

	VulkanUI& Start();
	VulkanUI& ShowSceneViewportPanel(Camera& scene_camera,
		VkDescriptorSet_T* texDeferred, VkDescriptorSet_T* texColorId,
		VkDescriptorSet_T* texNormalId, VkDescriptorSet_T* texDepthId,
		VkDescriptorSet_T* texNormalMapId, VkDescriptorSet_T* texMetallicRoughnessId);
	VulkanUI& ShowMenuBar();
	VulkanUI& AddHierarchyPanel();
	VulkanUI& AddInspectorPanel();
	VulkanUI& ShowStatistics(const char* title, float cpuDeltaSecs, size_t frameNumber);
	VulkanUI& ShowFrameTimeGraph(float* values, size_t nbValues);
	VulkanUI& ShowCameraSettings(Camera* camera);
	VulkanUI& ShowInspector();
	VulkanUI& ShowLightSettings(LightManager* light_manager);
	VulkanUI& ShowShadowPanel(CascadedShadowRenderer* shadow_renderer, std::span<size_t> texShadowCascades);
	void End();

	bool is_scene_viewport_hovered = false;
	float default_scene_view_aspect_ratio = 1.0f;
	float x, y;
	Camera *h_camera;

	/* To remove */
	glm::vec3 pos{0,0,0};
	glm::vec3 rot{0,0,0};
	glm::vec3 scale{1,1,1};
};

