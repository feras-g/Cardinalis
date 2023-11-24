#pragma once

#include "glm/gtx/log_base.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <span>

#include "Renderers/VulkanImGuiRenderer.h"
#include "Renderers/IRenderer.h"

class Camera;
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
	void show_toolbar();
	void show_inspector(const ObjectManager& object_manager);
	void show_camera_settings(Camera& camera);
	void show_draw_statistics(IRenderer::DrawStats draw_stats);
	void show_viewport_window(Camera& camera);
	void show_shader_library();

	/* UI rendering */
	void render(VkCommandBuffer cmd_buffer);
	void on_window_resize();
	const glm::vec2& get_render_area() const;

	
	bool is_scene_viewport_active();
	bool m_is_scene_viewport_hovered = false;
	float scene_view_aspect_ratio = 1.0f;
	static inline ImVec2 scene_viewport_window_size;
	/* State */
	VulkanGUI& ShowSceneViewportPanel(Camera& scene_camera,
		VkDescriptorSet_T* texDeferred, VkDescriptorSet_T* texColorId,
		VkDescriptorSet_T* texNormalId, VkDescriptorSet_T* texDepthId,
		VkDescriptorSet_T* texNormalMapId, VkDescriptorSet_T* texMetallicRoughnessId);
	VulkanGUI& ShowMenuBar();
	VulkanGUI& AddHierarchyPanel();
	VulkanGUI& AddInspectorPanel();
	static void start_overlay(const char* title);
	VulkanGUI& ShowFrameTimeGraph(float* values, size_t nbValues);
	VulkanGUI& ShowInspector();
	VulkanGUI& ShowLightSettings(LightManager* light_manager);

	bool is_scene_viewport_hovered = false;
	float x, y;
	Camera *h_camera;

private:
	VulkanImGuiRenderer m_renderer;
};

