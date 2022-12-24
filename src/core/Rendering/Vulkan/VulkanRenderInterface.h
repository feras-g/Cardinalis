#ifndef VULKAN_RENDER_INTERFACE_H
#define VULKAN_RENDER_INTERFACE_H

#include <memory>
#include <vector>

#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanSwapchain.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/VulkanTools.h"
#include "Rendering/Vulkan/VulkanFrame.h"
#include "Rendering/Vulkan/VulkanShader.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Window;

// Number of frames to work on 
constexpr uint32_t NUM_FRAMES = 2;

struct VulkanContext
{
	VkInstance instance			= VK_NULL_HANDLE;
	VkDevice   device			= VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice	= VK_NULL_HANDLE;
	VkQueue	   queue			= VK_NULL_HANDLE;
	VkCommandPool mainCmdPool	= VK_NULL_HANDLE;
	VkCommandBuffer mainCmdBuffer = VK_NULL_HANDLE;
	std::unique_ptr<VulkanSwapchain> swapchain;

	uint32_t frameCount= 0;
	uint32_t currentBackBuffer = 0; // From vkAcquireNextImageKHR
	VulkanFrame frames[NUM_FRAMES];
	uint32_t gfxQueueFamily		= 0;
};

extern VulkanContext context;

void BeginCommandBuffer(VkCommandBuffer cmdBuffer);
void EndCommandBuffer(VkCommandBuffer cmdBuffer);

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

	void CreateCommandStructures();
	void CreateSyncStructures();

	VulkanFrame& GetCurrentFrame();
	inline VulkanSwapchain* GetSwapchain() { return context.swapchain.get(); }

	inline size_t GetCurrentImageIdx() { return context.frameCount % NUM_FRAMES;  }
private:

private:
	std::vector<VkPhysicalDevice> vkPhysicalDevices;

	std::vector<const char*> deviceExtensions;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> instanceLayers;

	VkSurfaceKHR m_Surface;
	void CreateFramebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain);
	
	const char* m_Name;
	int minVer, majVer, patchVer;
};

// Render pass description

enum RenderPassFlags
{
	NONE			 = 0,
	RENDERPASS_FIRST = 1 << 1,
	RENDERPASS_LAST  = 1 << 2
};

struct RenderPassInitInfo
{
	// Clear attachments on initialization ?
	bool clearColor;
	bool clearDepth;

	// Is it the final pass ? 
	RenderPassFlags flags = NONE; 
};

bool CreateBuffer(VkDeviceSize sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkBuffer& out_Buffer, VkDeviceMemory& out_BufferMemory);
bool UploadBufferData(const VkDeviceMemory bufferMemory, VkDeviceSize offsetInBytes, const void* data, const size_t dataSizeInBytes);
bool CreateUniformBuffer(VkDeviceSize size, VkBuffer& out_Buffer, VkDeviceMemory& out_BufferMemory);
// Create a simple render pass with color and/or depth and a single subpass
bool CreateColorDepthRenderPass(const RenderPassInitInfo& rpi, bool useDepth, VkRenderPass* out_renderPass);
bool CreateColorDepthFramebuffers(VkRenderPass renderPass, VulkanSwapchain* swapchain, VkFramebuffer* out_Framebuffers, bool useDepth);

bool CreateDescriptorPool(uint32_t numStorageBuffers, uint32_t numUniformBuffers, uint32_t numCombinedSamplers, VkDescriptorPool* out_DescriptorPool);
bool CreateGraphicsPipeline(const VulkanShader& shader, bool useBlending, bool useDepth, VkPrimitiveTopology topology, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline* out_GraphicsPipeline, float customViewportWidth=0, float customViewportHeight=0);

// Create a storage buffer containing non-interleaved vertex and index data
// Return the created buffer's size 
size_t CreateIndexVertexBuffer(const void* vtxData, size_t vtxBufferSizeInBytes, const void* idxData, size_t idxBufferSizeInBytes, VkBuffer& out_StorageBuffer, VkDeviceMemory& out_StorageBufferMem);

// Create and execute a buffer copy command 
void CopyBuffer(VkBuffer srcBuffer, VkBuffer& dstBuffer, VkDeviceSize bufferSizeInBytes);

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint);
VkWriteDescriptorSet BufferWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType descriptorType);
VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo* imageInfo);

bool CreateTextureSampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler);
void BeginRenderpass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea = { 0, 0, context.swapchain->info.extent });
void EndRenderPass(VkCommandBuffer cmdBuffer);
void SetViewportScissor(VkCommandBuffer cmdBuffer, 
	VkViewport viewport = { 0,0, (float)context.swapchain->info.extent.width, (float)context.swapchain->info.extent.height, 0.0f, 1.0f } ,VkRect2D scissor = { 0,0, context.swapchain->info.extent });
bool CreatePipelineLayout(VkDevice device, VkDescriptorSetLayout descSetLayout, VkPipelineLayout* out_PipelineLayout);

void StartInstantUseCmdBuffer();
void EndInstantUseCmdBuffer();


#endif // !VULKAN_RENDER_INTERFACE_H
