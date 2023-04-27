#pragma once

#include <memory>
#include <vector>
#include <span>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#include "Core/EngineLogger.h"
#include "Rendering/Vulkan/VulkanResources.h"
#include "Rendering/Vulkan/VulkanSwapchain.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/VulkanTools.h"
#include "Rendering/Vulkan/VulkanFrame.hpp"
#include "Rendering/Vulkan/VulkanShader.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

static constexpr VkFormat ENGINE_SWAPCHAIN_COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
static constexpr VkFormat ENGINE_SWAPCHAIN_DS_FORMAT = VK_FORMAT_D32_SFLOAT;
static constexpr VkColorSpaceKHR ENGINE_SWAPCHAIN_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

class Window;

// Number of frames to work on 
constexpr uint32_t NUM_FRAMES = 2;

struct VulkanContext
{
	VkInstance instance			= VK_NULL_HANDLE;
	VkDevice   device			= VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice	= VK_NULL_HANDLE;
	VkQueue	   queue			= VK_NULL_HANDLE;
	VkCommandPool frames_cmd_pool = VK_NULL_HANDLE;
	VkCommandPool temp_cmd_pool	= VK_NULL_HANDLE;
	std::unique_ptr<VulkanSwapchain> swapchain;

	uint32_t frame_count= 0;
	uint32_t curr_frame_idx = 0;
	uint32_t curr_backbuffer_idx = 0; // From vkAcquireNextImageKHR
	VulkanFrame frames[NUM_FRAMES];
	uint32_t gfxQueueFamily		= 0;
};
extern VulkanContext context;

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

	inline size_t GetCurrentImageIdx() { return context.frame_count % NUM_FRAMES;  }

	static inline VkPhysicalDeviceLimits device_limits = {};
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

/* Function pointers*/
inline PFN_vkCmdBeginDebugUtilsLabelEXT		fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT		fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT	fpCmdInsertDebugUtilsLabelEXT;
inline PFN_vkSetDebugUtilsObjectNameEXT		fpSetDebugUtilsObjectNameEXT;
inline PFN_vkCmdBeginRenderingKHR			fpCmdBeginRenderingKHR;
inline PFN_vkCmdEndRenderingKHR				fpCmdEndRenderingKHR;

// Render pass description
enum RenderPassFlags
{
	NONE								= 0,
	RENDERPASS_FIRST					= 1 << 1,
	RENDERPASS_INTERMEDIATE_OFFSCREEN	= 1 << 2, // Output of the render pass will be used as a read-only input for a future render pass
	RENDERPASS_LAST						= 1 << 3
};

struct RenderPassInitInfo
{
	// Clear attachments on initialization ?
	bool clearColor = false;
	bool clearDepth = false;

	VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;

	// Is it the final pass ? 
	RenderPassFlags flags = NONE; 
};

VkCommandBuffer begin_temp_cmd_buffer();
void end_temp_cmd_buffer(VkCommandBuffer cmd_buffer);

void EndCommandBuffer(VkCommandBuffer cmdBuffer);
// Create a simple render pass with color and/or depth and a single subpass
bool CreateColorDepthRenderPass(const RenderPassInitInfo& rpi, VkRenderPass* out_renderPass);
bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const VulkanSwapchain* swapchain, VkFramebuffer* out_Framebuffers, bool useDepth);
bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const Texture2D* colorAttachments, const Texture2D* depthAttachments, VkFramebuffer* out_Framebuffers, bool useDepth);

VkDescriptorPool create_descriptor_pool(uint32_t num_ssbo, uint32_t num_ubo, uint32_t num_combined_img_smp, uint32_t num_dynamic_ubo, uint32_t max_sets = NUM_FRAMES);
VkDescriptorPool create_descriptor_pool(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets);

struct GraphicsPipeline
{
	enum Flags
	{
		NONE = 0,
		ENABLE_ALPHA_BLENDING	= 1 << 1,
		ENABLE_DEPTH_STATE		= 1 << 2,
		DISABLE_VTX_INPUT_STATE = 1 << 3
	};

	static bool CreateDynamic(const VulkanShader& shader, std::span<VkFormat> colorAttachmentFormats, VkFormat depthAttachmentFormat, Flags flags, VkPipelineLayout pipelineLayout,
		VkPipeline* out_GraphicsPipeline, VkCullModeFlags cullMode, VkFrontFace frontFace, glm::vec2 customViewport = {});
	static bool Create(const VulkanShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline* out_GraphicsPipeline,
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create = nullptr, glm::vec2 customViewport = {});
};

// Create a storage buffer containing non-interleaved vertex and index data
// Return the created buffer's size 
size_t CreateIndexVertexBuffer(Buffer& result, const void* vtxData, size_t vtxBufferSizeInBytes, const void* idxData, size_t idxBufferSizeInBytes);

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint);
VkWriteDescriptorSet BufferWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType descriptorType);
VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo* imageInfo, VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

bool CreateTextureSampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler);
void BeginRenderpass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount);

void EndRenderPass(VkCommandBuffer cmdBuffer);
void SetViewportScissor(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, bool invertViewportY = false);

VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> descSetLayout);
VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descSetLayout);
VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> desc_set_layouts, uint32_t vtxConstRangeSizeInBytes, uint32_t fragConstRangeSizeInBytes);

VkDescriptorSetLayout create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> layout_bindings);
VkDescriptorSet create_descriptor_set(VkDescriptorPool pool, VkDescriptorSetLayout layout);

void StartInstantUseCmdBuffer();
void EndInstantUseCmdBuffer();
