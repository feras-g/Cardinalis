#pragma once

#include <memory>
#include <vector>
#include <span>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#include "core/engine/EngineLogger.h"
#include "core/rendering/vulkan/VulkanResources.h"
#include "core/rendering/vulkan/VulkanSwapchain.h"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/engine/Image.h"
#include "core/rendering/vulkan/VulkanFrame.hpp"
#include "core/rendering/vulkan/VulkanShader.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>

#ifdef ENGINE_DEBUG
inline PFN_vkCmdBeginDebugUtilsLabelEXT		fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT		fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT	fpCmdInsertDebugUtilsLabelEXT;
inline PFN_vkSetDebugUtilsObjectNameEXT		fpSetDebugUtilsObjectNameEXT;
#endif // ENGINE_DEBUG
inline PFN_vkCmdBeginRenderingKHR			fpCmdBeginRenderingKHR;
inline PFN_vkCmdEndRenderingKHR				fpCmdEndRenderingKHR;

class Window;

// Number of frames to work on 
static constexpr uint32_t NUM_FRAMES = 2;

struct VulkanContext
{
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physical_device;
	VkQueue queue;
	VkCommandPool frames_cmd_pool;
	VkCommandPool temp_cmd_pool;
	VulkanFrame frames[NUM_FRAMES];

	VulkanFrame& get_current_frame() { return frames[curr_frame_idx]; }
	void update_frame_index() { curr_frame_idx = (curr_frame_idx + 1) % NUM_FRAMES; }

	std::unique_ptr<VulkanSwapchain> swapchain;

	uint32_t frame_count= 0;
	uint32_t curr_frame_idx = 0;
	uint32_t curr_backbuffer_idx = 0; 
	uint32_t gfxQueueFamily		= 0;
};
extern VulkanContext context;

class RenderInterface
{
public:
	RenderInterface(const char* name, int maj, int min, int patch);

	void initialize();
	void terminate();
	bool m_init_success;

	void create_instance();
	void create_device();
	void create_surface(Window* window);
	void create_swapchain();

	void create_command_structures();
	void create_synchronization_structures();

	VulkanFrame& get_current_frame();
	inline VulkanSwapchain* get_swapchain() const { return context.swapchain.get(); }

	inline size_t get_current_frame_index() const { return context.frame_count % NUM_FRAMES;  }

	static inline VkPhysicalDeviceLimits device_limits = {};

	VkSurfaceKHR surface;

private:

	std::vector<const char*> extensions;
	void create_framebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain);

	const char* m_name;
	int min_ver, maj_ver, patch_ver;
};



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

VkDescriptorPool create_descriptor_pool(VkDescriptorPoolCreateFlags flags, uint32_t num_ssbo, uint32_t num_ubo, uint32_t num_combined_img_smp, uint32_t num_dynamic_ubo, uint32_t num_storage_image, uint32_t max_sets = NUM_FRAMES);
VkDescriptorPool create_descriptor_pool(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets);

struct Pipeline
{
	enum Flags
	{
		NONE = 1 << 0,
		ENABLE_ALPHA_BLENDING	= 1 << 1,
		ENABLE_DEPTH_STATE		= 1 << 2,
		DISABLE_VTX_INPUT_STATE = 1 << 3
	};

	struct Layout
	{
		operator VkPipelineLayout() { return vk_pipeline_layout; }

		void add_push_constant_range(const std::string& name, VkPushConstantRange range)
		{
			ranges[name] = range;
			push_constant_ranges.push_back(range);
		}

		void cmd_push_constants(VkCommandBuffer cmd_buffer, std::string_view push_constant_range_name, const void* p_values)
		{
			if (ranges.contains(push_constant_range_name.data()))
			{
				VkPushConstantRange& range = ranges.at(push_constant_range_name.data());
				vkCmdPushConstants(cmd_buffer, vk_pipeline_layout, range.stageFlags, range.offset, range.size, p_values);
			}
			else
			{
				assert(false);
			}
		}

		void create(std::span<VkDescriptorSetLayout> set_layouts)
		{
			VkPipelineLayoutCreateInfo pipeline_layout_info = {};
			pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
			pipeline_layout_info.pSetLayouts = set_layouts.data();
			pipeline_layout_info.pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data();
			pipeline_layout_info.pushConstantRangeCount = (uint32_t)push_constant_ranges.size();

			vkCreatePipelineLayout(context.device, &pipeline_layout_info, nullptr, &vk_pipeline_layout);
		}

		std::unordered_map<std::string, VkPushConstantRange> ranges;
		std::vector<VkPushConstantRange> push_constant_ranges;

		VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;

		
	} layout;

	VkPipeline pipeline;

	operator VkPipeline() { return pipeline; }

	void create_graphics(const VertexFragmentShader& shader, std::span<VkFormat> color_formats, VkFormat depth_format, Flags flags, VkPipelineLayout pipeline_layout,
		VkPrimitiveTopology topology, VkCullModeFlags cull_mode, VkFrontFace front_face, uint32_t view_mask = 0);
	void create_graphics(const VertexFragmentShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPrimitiveTopology topology,
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create);
	void create_compute(const Shader& shader);
};

static Pipeline::Flags operator|(Pipeline::Flags a, Pipeline::Flags b)
{
	return static_cast<Pipeline::Flags>(static_cast<int>(a) | static_cast<int>(b));
}

// Create a storage buffer containing non-interleaved vertex and index data
// Return the created buffer's size 
// May modify sizes to comply to SSBO alignment
size_t create_vertex_index_buffer(Buffer& result, const void* vtxData, size_t& vtxBufferSizeInBytes, const void* idxData, size_t& idxBufferSizeInBytes);

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint);
VkWriteDescriptorSet BufferWriteDescriptorSet(VkDescriptorSet descriptor_set, uint32_t binding, const VkDescriptorBufferInfo& desc_info, VkDescriptorType desc_type);
VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet& descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo& imageInfo, VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t array_offset = 0, uint32_t descriptor_count = 1);

void create_sampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler);
void BeginRenderpass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount);

void EndRenderPass(VkCommandBuffer cmdBuffer);
void set_viewport_scissor(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, bool invertViewportY = false);

VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> descSetLayout, std::span<VkPushConstantRange> push_constant_ranges = {});
VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descSetLayout, std::span<VkPushConstantRange> push_constant_ranges );

VkDescriptorSetLayout create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> layout_bindings, VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindings_flags = {});

VkDescriptorSet create_descriptor_set(VkDescriptorPool pool, VkDescriptorSetLayout layout);