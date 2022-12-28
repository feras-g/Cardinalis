#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>

class Window;
class VulkanRenderInterface;
class FrameCounter;

// Renderers forward decls
class VulkanRendererBase;
class VulkanClearColorRenderer;
class VulkanImGuiRenderer;
class VulkanModelRenderer;
class VulkanPresentRenderer;

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
	virtual void UpdateGuiData(size_t currentImageIdx) = 0;
	virtual void UpdateRenderersData(size_t currentImageIdx) = 0;
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
	double m_LastTime = 0.0f, m_DeltaSeconds = 0.0f;
	const char* m_DebugName;
	std::unique_ptr<FrameCounter> m_FramePerfCounter;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanRenderInterface> m_RHI;

	// Renderers
	std::unique_ptr<VulkanClearColorRenderer>	m_ClearRenderer;
	std::unique_ptr<VulkanImGuiRenderer>		m_ImGuiRenderer;
	std::unique_ptr<VulkanModelRenderer>		m_ModelRenderer;
	std::unique_ptr<VulkanPresentRenderer>		m_PresentRenderer;
};

#endif // !APPLICATION_H

