#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>

class Window;
class VulkanRenderInterface;
class VulkanRendererBase;

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
	virtual void Update(/* float delta */) = 0;
	void PreRender();
	void PostRender();
	virtual void Render(size_t currentImageIdx) = 0;
	virtual void RenderGUI(size_t currentImageIdx) = 0;
	virtual void Terminate() = 0;
	void OnWindowResize();

	bool bInitSuccess;

public:
	Application() = delete;
	Application(const Application&) = delete;
	Application(const Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(const Application&&) = delete;

protected:
	const char* m_DebugName;
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanRenderInterface> m_RHI;
	std::vector<std::unique_ptr<VulkanRendererBase>> renderers;
};

#endif // !APPLICATION_H

