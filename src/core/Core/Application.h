#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

class Window;
class VulkanRenderInterface;

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
	virtual void Render() = 0;
	virtual void Terminate() = 0;

public:
	Application() = delete;
	Application(const Application&) = delete;
	Application(const Application&&) = delete;
	Application& operator=(const Application&) = delete;
	Application& operator=(const Application&&) = delete;

protected:
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanRenderInterface> m_RHI;
};

#endif // !APPLICATION_H

