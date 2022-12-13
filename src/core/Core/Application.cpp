#include "Application.h"
#include "Window/Window.h"

#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"

Application::Application(const char* title, uint32_t width, uint32_t height)
{
	m_Window.reset(new Window({ .title = title, .width = width, .height = height }, this));
	Logger::Init("EnginerLogger");

	m_RHI.reset(new VulkanRenderInterface(title, 1, 3, 283));
	m_RHI->Initialize();
#ifdef _WIN32
	m_RHI->CreateSurface(m_Window.get());
	m_RHI->CreateSwapchain();
#else
	assert(false);
#endif // _WIN32
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
		m_Window->HandleEvents();
		Update();
		PreRender();	
		RenderGUI(context.currentBackBuffer);
		Render(context.currentBackBuffer);
		PostRender();
	}

	Terminate();
}

void Application::PreRender()
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const VulkanFrame& currentFrame = m_RHI->GetCurrentFrame();
	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	VK_CHECK(vkWaitForFences(context.device, 1, &currentFrame.renderFence, true, OneSecondInNanoSeconds));
	VK_CHECK(vkResetFences(context.device, 1, &currentFrame.renderFence));

	VkResult result = swapchain.AcquireNextImage(currentFrame.imageAcquiredSemaphore, &context.currentBackBuffer);

	if ( result == VK_ERROR_OUT_OF_DATE_KHR)
	{ 
		OnWindowResize(); 
	}
}

void Application::PostRender()
{
	// Submit commands for the GPU to work on the current backbuffer
	// Has to wait for the swapchain image to be acquired before beginning, we wait on imageAcquired semaphore.
	// Signals a renderComplete semaphore to let the next operation know that it finished
	const VulkanFrame& currentFrame = m_RHI->GetCurrentFrame();
	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentFrame.imageAcquiredSemaphore,
		.pWaitDstStageMask = &waitDstStageMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &currentFrame.cmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentFrame.renderCompleteSemaphore
	};
	vkQueueSubmit(context.queue, 1, &submitInfo, currentFrame.renderFence);

	// Present work
	// Waits for the GPU queue to finish execution before presenting, we wait on renderComplete semaphore
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentFrame.renderCompleteSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &context.currentBackBuffer
	};
	
	VkResult result = vkQueuePresentKHR(context.queue, &presentInfo);

	if (result == VK_SUBOPTIMAL_KHR) 
	{ 
		OnWindowResize(); 
	}

	context.frameCount++;
}

void Application::OnWindowResize()
{
	context.swapchain->info.extent = {  (unsigned int)m_Window->GetWidth(), (unsigned int)m_Window->GetHeight() };
	context.swapchain->Reinitialize();

	for (int i = 0; i < renderers.size(); i++)
	{
		renderers[i]->RecreateFramebuffersRenderPass();
	}
}
