#include "engine/Application.h"
#include "rendering/Camera.h"
#include "rendering/vulkan/VulkanUI.h"
#include "rendering/vulkan/VulkanRendererBase.h"
#include "rendering/vulkan/RenderObjectManager.h"

/*
*	Example project with a simple GUI and a forward mesh renderer.
*/

class SampleProject : public Application
{
public:
	SampleProject(const char* title, uint32_t width, uint32_t height);

	virtual void init() override;
	virtual void create_scene();
	virtual void compose_gui() override;
	virtual void render() override;
	virtual void update(float t, float dt) override;

	/* t : time in seconds */
	virtual void update_scene(float t, float dt);
	virtual void update_frame_ubo();
	virtual void exit() override;

	virtual void on_window_resize() override;
	virtual void on_mouse_move(MouseEvent event) override;
	virtual void on_key_event(KeyEvent event) override;
	virtual void on_lmb_down(MouseEvent event) override;
	
private:
	VulkanGUI m_gui;
	Camera m_camera;
};