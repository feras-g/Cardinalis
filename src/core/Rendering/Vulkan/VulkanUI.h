#pragma once

#include "glm/gtx/log_base.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <span>

#include "Renderers/VulkanImGuiRenderer.h"
#include "Renderers/IRenderer.h"

class camera;
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
	void show_toolbar();
	void show_hierarchy(ObjectManager& object_manager);
	void show_draw_statistics(IRenderer::DrawStats draw_stats);
	void show_viewport_window(ImTextureID scene_image_id, camera& camera, ObjectManager& object_manager);
	void show_shader_library();
	void start_overlay(const char* title);
	
	/* UI rendering */
	void render(VkCommandBuffer cmd_buffer);
	void on_window_resize();
	const glm::vec2& get_render_area() const;

	bool is_inactive();
	
	bool b_is_scene_viewport_active = false;
	float viewport_aspect_ratio = 1.0;

	bool is_not_selecting_gizmo() const;
	bool is_scene_viewport_active() const;
	float x, y;
	camera *h_camera;

private:
	VulkanImGuiRenderer m_renderer;
};

