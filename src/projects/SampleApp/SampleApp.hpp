#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

#include "Core/Application.h"
#include "Window/Window.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/Renderers/VulkanPresentRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/VulkanClearColorRenderer.h"
#include "Rendering/FrameCounter.h"

class SampleApp final : public Application
{
public:
	SampleApp() : Application("SampleApp", 1280, 720)
	{

	}
	~SampleApp();
	void Initialize()	override;
	void Update()		override;
	void Render(size_t currentImageIdx)		override;
	void UpdateGuiData(size_t currentImageIdx)	override;
	void UpdateRenderersData(size_t currentImageIdx) override;
	void OnWindowResize() override;
	void Terminate()	override;


protected:
	std::unique_ptr<VulkanImGuiRenderer>		m_ImGuiRenderer;
	std::unique_ptr<VulkanModelRenderer>		m_ModelRenderer;
	std::unique_ptr<VulkanPresentRenderer>		m_PresentRenderer;
	std::unique_ptr<VulkanClearColorRenderer>	m_ClearColorRenderer;
};

void SampleApp::Initialize()
{
	m_PresentRenderer.reset(new VulkanPresentRenderer(context, false));
	m_ModelRenderer.reset(new VulkanModelRenderer ("../../../data/models/suzanne.obj", "../../../data/textures/default.png"));
	m_ImGuiRenderer.reset(new VulkanImGuiRenderer(context));
	m_ImGuiRenderer->Initialize(m_ModelRenderer->m_ColorAttachments);
	m_ClearColorRenderer.reset(new VulkanClearColorRenderer(context));
}

void SampleApp::Update()
{
	// Update keyboard, mouse interaction
	m_Window->UpdateGUI();
	UpdateRenderersData(context.currentBackBuffer);
}

void SampleApp::Render(size_t currentImageIdx)
{
	VulkanFrame& currentFrame = context.frames[context.frameCount % NUM_FRAMES];
	VulkanSwapchain& swapchain = *m_RHI->GetSwapchain();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PRE-RENDER
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*VK_CHECK*/(vkWaitForFences(context.device, 1, &currentFrame.renderFence, true, OneSecondInNanoSeconds));
	/*VK_CHECK*/(vkResetFences(context.device, 1, &currentFrame.renderFence));

	VkResult result = swapchain.AcquireNextImage(currentFrame.imageAcquiredSemaphore, &context.currentBackBuffer);

	// Populate command buffers
	{	
		VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		VK_CHECK(vkResetCommandPool(context.device, currentFrame.cmdPool, 0));
		VK_CHECK(vkBeginCommandBuffer(currentFrame.cmdBuffer, &cmdBufferBeginInfo));

		m_ClearColorRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_ModelRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_ImGuiRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);
		m_PresentRenderer->PopulateCommandBuffer(context.currentBackBuffer, currentFrame.cmdBuffer);

		VK_CHECK(vkEndCommandBuffer(currentFrame.cmdBuffer));
	}

	// Submit commands for the GPU to work on the current backbuffer
	// Has to wait for the swapchain image to be acquired before beginning, we wait on imageAcquired semaphore.
	// Signals a renderComplete semaphore to let the next operation know that it finished

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
		.pSignalSemaphores = &currentFrame.queueSubmittedSemaphore
	};
	vkQueueSubmit(context.queue, 1, &submitInfo, currentFrame.renderFence);

	// Present work
	// Waits for the GPU queue to finish execution before presenting, we wait on renderComplete semaphore
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentFrame.queueSubmittedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &context.currentBackBuffer
	};

	vkQueuePresentKHR(context.queue, &presentInfo);

	context.frameCount++;
}

inline void SampleApp::UpdateGuiData(size_t currentImageIdx)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::NewFrame();
	ImGui::StyleColorsDark();

	// Viewport
	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

	// Scene view
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
	if (ImGui::Begin("Scene", 0, 0))
	{
		ImVec2 sceneViewPanelSize = ImGui::GetContentRegionAvail();

		m_ImGuiRenderer->m_SceneViewAspectRatio = sceneViewPanelSize.x / sceneViewPanelSize.y;
		ImGui::Image((ImTextureID)m_ImGuiRenderer->m_ModelRendererColorTextureId[currentImageIdx], sceneViewPanelSize);
	}
	ImGui::End();
	ImGui::PopStyleVar();
	
	// Hierachy panel
	if (ImGui::Begin("Hierarchy", 0, 0))
	{

	}
	ImGui::End();

	// Toolbar
	// TODO

	// Inspector panel
	if (ImGui::Begin("Inspector", 0, 0))
	{

	}
	ImGui::End();


	// Overlay
	ImGuiWindowFlags overlayFlags = 
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | 
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	const float PAD = 10.0f;
	
	ImVec2 work_pos  = viewport->WorkPos;
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = (work_pos.x + PAD);
	window_pos.y = (work_pos.y + work_size.y - PAD);

	window_pos_pivot.x = 0.0f;
	window_pos_pivot.y = 1.0f;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	ImGui::SetNextWindowBgAlpha(0.33f); // Transparent background
	if (ImGui::Begin("Overlay", 0, overlayFlags))
	{
		ImGui::Text(m_DebugName);
		ImGui::AlignTextToFramePadding();
		const float s = Application::m_DeltaSeconds;
		ImGui::Text("CPU : %.2f ms (%.1f fps)", s * 1000.0f, 1.0f / s);
	}
	ImGui::End();

	ImGui::Render();
}

inline void SampleApp::UpdateRenderersData(size_t currentImageIdx)
{
	// Update ImGUI buffers
	UpdateGuiData(context.currentBackBuffer);

	// Other renderers data
	{
		glm::mat4 m = glm::identity<glm::mat4>();
		glm::mat4 v = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 p = glm::perspective(45.0f, m_ImGuiRenderer->m_SceneViewAspectRatio, 0.1f, 1000.0f);

		m_ModelRenderer->UpdateBuffers(currentImageIdx, m, v, p);
	}
}

inline void SampleApp::OnWindowResize()
{
	context.swapchain->Reinitialize();
	m_ClearColorRenderer->CreateFramebufferAndRenderPass();
	m_PresentRenderer->RecreateFramebuffersRenderPass();
	m_ImGuiRenderer->RecreateFramebuffersRenderPass();
}

inline void SampleApp::Terminate()
{
	m_Window->ShutdownGUI();
}

inline SampleApp::~SampleApp()
{
}

#endif // !SAMPLE_APP_H

