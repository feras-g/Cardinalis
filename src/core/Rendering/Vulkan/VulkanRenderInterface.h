#ifndef VULKAN_RENDER_INTERFACE
#define VULKAN_RENDER_INTERFACE

#include <memory>
#include <vector>

#include "Core/EngineLogger.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Vulkan/VulkanSwapchain.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/VulkanTools.h"
#include "Rendering/Vulkan/VulkanFrame.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

class Window;

// Number of frames to work on 
constexpr uint32_t NUM_FRAMES = 2;

struct VulkanContext
{
	VkInstance instance			= VK_NULL_HANDLE;
	VkDevice device				= VK_NULL_HANDLE;
	VkQueue queue				= VK_NULL_HANDLE;
	VkCommandPool mainCmdPool	= VK_NULL_HANDLE;
	VkCommandBuffer mainCmdBuffer = VK_NULL_HANDLE;

	uint32_t frameNumber = 0;
	uint32_t currentBackBuffer = 0; // From vkAcquireNextImageKHR
	VulkanFrame frames[NUM_FRAMES];
	uint32_t gfxQueueFamily		= 0;
};

extern VulkanContext context;

void BeginRecording(VkCommandBuffer cmdBuffer);
void EndRecording(VkCommandBuffer cmdBuffer);

class VulkanRenderInterface
{
public:
	VulkanRenderInterface(const char* name, int maj, int min, int patch);

	void Initialize();
	void Terminate();
	bool m_InitSuccess;

	void CreateInstance();
	void CreateDevices();
	void CreateSurface(Window* window);
	void CreateSwapchain();
	VkRenderPass CreateExampleRenderPass();
	void CreateFramebuffers(VkRenderPass renderPass);

	void CreateCommandStructures();
	void CreateSyncStructures();

	VulkanFrame& GetCurrentFrame();
	inline VulkanSwapchain* GetSwapchain() { return swapchain.get(); }
private:


private:
	std::vector<VkPhysicalDevice> vkPhysicalDevices;

	std::vector<const char*> deviceExtensions;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> instanceLayers;

	VkSurfaceKHR m_Surface;
	std::unique_ptr<VulkanSwapchain> swapchain;
	void CreateFramebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain);
	
	const char* m_Name;
	int minVer, majVer, patchVer;
};
#endif // !VULKAN_RENDER_INTERFACE


