#pragma once

#include <memory>
#include <vector>

#include "InputEvents.h"

class Window;
class VulkanRenderInterface;
class FrameCounter;
struct VulkanFrame;

/* Renderers forward decls */
class VulkanRendererBase;
class VulkanImGuiRenderer;
class VulkanModelRenderer;
class VulkanClearColorRenderer;

/// <summary>
/// Skeleton of an application to derive in client code.
/// </summary>
class Application
{
public:
	/// <summary>
	/// Initialize the rendering context and create a window.
	/// </summary>
	/// <param name="title">Window title</param>
	/// <param name="width">Window width</param>
	/// <param name="height">Window height</param>
	Application(const char* title, uint32_t width, uint32_t height);
	virtual ~Application();
	void Run();

	/*
	*	Returns time in seconds.
	*/
	double GetTime();

	/// <summary>
	/// Application specific functions to derive in client code.
	/// </summary>
	virtual void Initialize() = 0;

	/* 
	*	Main update loop for the application.
	* 
	*	@param t	Time in seconds 
	*	@param dt	Delta time in seconds
	*/
	virtual void Update(float t, float dt) = 0;

	/*
	*	Main render loop for the application.
	*/
	virtual void Render() = 0;
	virtual void UpdateRenderersData(float dt, size_t currentImageIdx) = 0;
	virtual void Terminate() = 0;

	/// <summary>
	/// States callbacks
	/// </summary>
	bool EngineGetAsyncKeyState(Key key);
	virtual void OnWindowResize();
	virtual void OnLeftMouseButtonUp();
	virtual void OnLeftMouseButtonDown();
	virtual void OnMouseMove(int x, int y);
	virtual void OnKeyEvent(KeyEvent event);

	bool bInitSuccess;
	
public:
	Application() = delete;
	Application(const Application&) = delete;
	Application(const Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(const Application&&) = delete;

protected:
	float m_LastFrameTime {};
	MouseEvent m_MouseEvent;
	KeyEvent   m_KeyEvent;

	const char* m_DebugName;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanRenderInterface> m_RHI;
};

