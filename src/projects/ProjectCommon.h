#include "engine/Application.h"
#include "engine/InputEvents.h"
#include "rendering/vulkan/VulkanUI.h"
#include "rendering/vulkan/VulkanRendererBase.h"

/*
*	Class with common functions to build a project.
*/

class ProjectCommon : public Application
{
public:
	ProjectCommon(const char* title, uint32_t width, uint32_t height);

	virtual void init() override;
	virtual void create_scene();
	virtual void compose_gui() override;
	virtual void render() override;
	/* t : time in seconds */
	virtual void update(float t, float dt) override;
	virtual void update_gpu_buffers() override;
	virtual void update_instances_ssbo();
	virtual void update_frame_ubo();
	virtual void exit() override;

	virtual void on_window_resize() override;
	virtual void on_mouse_move(MouseEvent event) override;
	virtual void on_key_event(KeyEvent event) override;
	virtual void on_mouse_up(MouseEvent event) override;
	virtual void on_mouse_down(MouseEvent event) override;

private:
	VulkanGUI m_gui;
};