#pragma once

#include <memory>
#include <vector>
#include <span>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include "core/engine/common.h"
#include "core/engine/vulkan/vk_common.h"
#include "core/engine/vulkan/vk_context.h"
#include "core/engine/vulkan/objects/vk_device.h"

#include "core/engine/logger.h"

#include <vulkan/vk_enum_string_helper.h>

#include "core/engine/vulkan/objects/vk_buffer.h"
#include "core/engine/vulkan/objects/vk_swapchain.h"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/engine/Image.h"
#include "core/rendering/vulkan/VulkanShader.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>

//inline PFN_vkCmdSetPolygonModeEXT			fpCmdSetPolygonModeEXT;

class Window;

class RenderInterface
{
public:
	RenderInterface(const char* name, int maj, int min, int patch);

	void init();
	void terminate();
	bool m_init_success;

	void create_device();
	void create_surface(Window* window);
	void create_swapchain();

	void create_command_structures();
	void create_synchronization_structures();

	vk::frame& get_current_frame();
	inline vk::swapchain* get_swapchain() const { return ctx.swapchain.get(); }

	inline size_t get_current_frame_index() const { return ctx.frame_count % NUM_FRAMES;  }

	static inline VkPhysicalDeviceLimits device_limits = {};

	VkSurfaceKHR surface;

private:

	std::vector<const char*> extensions;
	void create_framebuffers(VkRenderPass renderPass, vk::swapchain& swapchain);

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
bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const vk::swapchain* swapchain, VkFramebuffer* out_Framebuffers, bool useDepth);
bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const Texture2D* colorAttachments, const Texture2D* depthAttachments, VkFramebuffer* out_Framebuffers, bool useDepth);

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

			vkCreatePipelineLayout(ctx.device, &pipeline_layout_info, nullptr, &vk_pipeline_layout);
		}

		std::unordered_map<std::string, VkPushConstantRange> ranges;
		std::vector<VkPushConstantRange> push_constant_ranges;

		VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;

	} layout;

	VkPipeline pipeline;

	operator VkPipeline() { return pipeline; }

	void create_graphics(const VertexFragmentShader& shader, std::span<VkFormat> color_formats, VkFormat depth_format, Flags flags, VkPipelineLayout pipeline_layout,
		VkPrimitiveTopology topology, VkCullModeFlags cull_mode, VkFrontFace front_face, uint32_t view_mask = 0, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
	void create_graphics(const VertexFragmentShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPrimitiveTopology topology,
		VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create,
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
	void create_compute(const Shader& shader);
	bool reload_pipeline();

	void bind(VkCommandBuffer cmd_buffer) const;

	bool is_graphics = false;

	/* Saved pipeline info */
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	VkPipelineRenderingCreateInfoKHR dynamic_pipeline_create_info = {};
	VertexFragmentShader pipeline_shader;
	std::vector<VkFormat> color_attachment_formats;
	VkFormat depth_attachment_format;
	Flags pipeline_flags;

	std::vector<VkDynamicState> pipeline_dynamic_states 
	{ 
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
	};
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};

	VkPipelineVertexInputStateCreateInfo vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	VkPipelineMultisampleStateCreateInfo multisampleState;
	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	VkPipelineColorBlendStateCreateInfo colorBlendState;
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkPipelineViewportStateCreateInfo viewportState = {};
	VkPipelineRasterizationStateCreateInfo raster_state_info = {};
};

static Pipeline::Flags operator|(Pipeline::Flags a, Pipeline::Flags b)
{
	return static_cast<Pipeline::Flags>(static_cast<int>(a) | static_cast<int>(b));
}

// Create a storage buffer containing non-interleaved vertex and index data
// Return the created buffer's size 
// May modify sizes to comply to SSBO alignment
template<typename T>
size_t create_vertex_index_buffer(vk::buffer& result, std::span<T> vtxData, size_t& vtxBufferSizeInBytes, std::span<T> idxData, size_t& idxBufferSizeInBytes);

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint);

void create_sampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler);
void BeginRenderpass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount);

void EndRenderPass(VkCommandBuffer cmdBuffer);
void set_viewport_scissor(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, bool flip_y = false);
void set_polygon_mode(VkCommandBuffer cmdBuffer, VkPolygonMode mode);

VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> descSetLayout, std::span<VkPushConstantRange> push_constant_ranges = {});
VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descSetLayout, std::span<VkPushConstantRange> push_constant_ranges );
