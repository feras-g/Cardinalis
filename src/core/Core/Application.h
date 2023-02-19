#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>

#include "InputEvents.h"

class Window;
class VulkanRenderInterface;
class FrameCounter;

// Renderers forward decls
class VulkanRendererBase;
class VulkanImGuiRenderer;
class VulkanModelRenderer;
class VulkanPresentRenderer;
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

	/// <summary>
	/// Application specific functions to derive in client code.
	/// </summary>
	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;
	virtual void Render(size_t currentImageIdx) = 0;
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
	float m_LastFrameTime;
	float lastTimeSteps[500];
	MouseEvent m_MouseEvent;
	KeyEvent   m_KeyEvent;

	const char* m_DebugName;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanRenderInterface> m_RHI;
};

#endif // !APPLICATION_H

