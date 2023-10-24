#pragma once

#include "glm/gtx/log_base.hpp"
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <span>
#include "Renderers/VulkanImGuiRenderer.h"

class Camera;
struct CascadedShadowRenderer;
struct KeyEvent;
struct LightManager;
class ObjectManager;
struct VkDescriptorSet_T;

class VulkanGUI
{
public:
	VulkanGUI();

	/* UI composition functions */
	void begin();
	void show_demo();
	void end();
	void exit();
	void show_gizmo(const Camera& camera, const KeyEvent& event, glm::mat4& selected_object_transform);
	void show_inspector(const ObjectManager& object_manager);
	void show_camera_settings(Camera& camera);

	/* UI rendering */
	void render(VkCommandBuffer cmd_buffer);
	void on_window_resize();
	const glm::vec2& get_render_area() const;

	/* State */
	bool is_in_focus();

	VulkanGUI& ShowSceneViewportPanel(Camera& scene_camera,
		VkDescriptorSet_T* texDeferred, VkDescriptorSet_T* texColorId,
		VkDescriptorSet_T* texNormalId, VkDescriptorSet_T* texDepthId,
		VkDescriptorSet_T* texNormalMapId, VkDescriptorSet_T* texMetallicRoughnessId);
	VulkanGUI& ShowMenuBar();
	VulkanGUI& AddHierarchyPanel();
	VulkanGUI& AddInspectorPanel();
	VulkanGUI& show_overlay(const char* title, float cpuDeltaSecs, size_t frameNumber);
	VulkanGUI& ShowFrameTimeGraph(float* values, size_t nbValues);
	VulkanGUI& ShowInspector();
	VulkanGUI& ShowLightSettings(LightManager* light_manager);
	VulkanGUI& ShowShadowPanel(CascadedShadowRenderer* shadow_renderer, std::span<VkDescriptorSet_T*> texShadowCascades);

	bool is_scene_viewport_hovered = false;
	float default_scene_view_aspect_ratio = 1.0f;
	float x, y;
	Camera *h_camera;

	/* To remove */
	glm::vec3 pos{0,0,0};
	glm::vec3 rot{0,0,0};
	glm::vec3 scale{1,1,1};
private:
	VulkanImGuiRenderer m_renderer;
};

