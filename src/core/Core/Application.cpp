#include "Application.h"
#include "Window/Window.h"
#include <iostream>
#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanFrame.hpp"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/FrameCounter.h"

Application::Application(const char* title, uint32_t width, uint32_t height) : bInitSuccess(false), m_DebugName(title)
{
	m_Window.reset(new Window({ .title = title, .width = width, .height = height }, this));
	Logger::Init("Engine");

	m_RHI.reset(new VulkanRenderInterface(title, 1, 2, 196));
	m_RHI->Initialize();
#ifdef _WIN32
	m_RHI->CreateSurface(m_Window.get());
	m_RHI->CreateSwapchain();
#else
	assert(false);
#endif // _WIN32

	bInitSuccess = true;
}

Application::~Application()
{
	m_RHI->Terminate();
	m_RHI.release();
	m_Window.release();
}

void Application::Run()
{
	Initialize();
	while (!m_Window->IsClosed())
	{
		float time = (float)m_Window->GetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;
		m_Window->HandleEvents();
		FrameStats::UpdateFrameTimeHistory(timestep.GetSeconds());
		Update(timestep.GetSeconds());
		Render(context.currentBackBuffer, context.frames[context.currentBackBuffer]);
	}

	Terminate();
}

void Application::OnWindowResize()
{

}

void Application::OnLeftMouseButtonDown()
{
	m_MouseEvent.bLeftClick = true;
	m_MouseEvent.bFirstClick = true;

	LOG_INFO("LMB clicked");
}

void Application::OnLeftMouseButtonUp()
{
	m_MouseEvent.bLeftClick = false;

	LOG_INFO("LMB released");
}

void Application::OnMouseMove(int x, int y)
{
	if (m_MouseEvent.bLeftClick)
	{
		m_MouseEvent.bFirstClick = false;

		LOG_INFO("LMB + Dragging mouse", x, y);
	}

	m_MouseEvent.px = x;
	m_MouseEvent.py = y;


	//LOG_INFO("Mouse position : {0} {1}", x, y);
}

bool Application::EngineGetAsyncKeyState(Key key)
{
	return m_Window->AsyncKeyState(key);
}

void Application::OnKeyEvent(KeyEvent event)
{
	if (event.pressed)
	{
		LOG_INFO("Key pressed : {0}", keyToString(event.key).c_str());
	}
	else
	{
		LOG_ERROR("Key released");
	}
}
