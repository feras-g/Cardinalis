#pragma once

#include <memory>
#include <vector>

#include "InputEvents.h"

class Window;
class RenderInterface;
class FrameCounter;
struct VulkanFrame;

/* Renderers forward decls */
class VulkanRendererCommon;
class VulkanImGuiRenderer;
class VulkanModelRenderer;
class VulkanClearColorRenderer;

/// <summary>
/// Skeleton of an application to derive in client code.
/// </summary>
class Application {
public:
	
	Application(const char* title, uint32_t width, uint32_t height);
	virtual ~Application();

	/* Main application loop */
	void run();

	/* Project specific functions to derive. */
	virtual void init() = 0;
	virtual void compose_gui() = 0;
	virtual void update(float t, float dt) = 0;
	virtual void render() = 0;
	virtual void exit() = 0;
	virtual void update_gpu_buffers() = 0;

	/* Prepare for render */
	void prerender();

	/* Submit render commands and prepare for next render */
	void postrender();

	/* Input/Key/Events */
	bool get_key_state(Key key);
	virtual void on_window_resize();
	virtual void on_lmb_up(MouseEvent event);
	virtual void on_lmb_down(MouseEvent event);
	virtual void on_mouse_move(MouseEvent event);
	virtual void on_key_event(KeyEvent event);

	/*
	 *	Returns time in seconds.
	 */
	double get_time_secs();

	bool b_init_success;

	std::unique_ptr<EventManager> m_event_manager;

	uint32_t m_backbuffer_idx = 0;

public:
	Application() = delete;
	Application(const Application&) = delete;
	Application(const Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(const Application&&) = delete;

protected:
	float m_last_frametime;
	float m_delta_time;
	float m_time;

	const char* m_debug_name;

	std::unique_ptr<Window> m_window;
	std::unique_ptr<RenderInterface> m_rhi;
};

