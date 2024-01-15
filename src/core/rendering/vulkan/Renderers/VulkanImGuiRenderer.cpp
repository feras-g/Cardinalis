#include "VulkanImGuiRenderer.h"
#include "core/engine/vulkan/objects/vk_debug_marker.hpp"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanShader.h"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/engine/vulkan/objects/vk_descriptor_set.hpp"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING

using namespace vk;

void VulkanImGuiRenderer::init()
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}, 
	};

	m_descriptor_pool = create_descriptor_pool(pool_sizes, 64);

	ImGui_ImplVulkan_InitInfo imgui_vk_info = {};

	imgui_vk_info.Instance = ctx.device.instance;
	imgui_vk_info.PhysicalDevice = ctx.device.physical_device;
	imgui_vk_info.Device = ctx.device;
	imgui_vk_info.QueueFamily = ctx.device.queue_family_indices[vk::queue_family::graphics];
	imgui_vk_info.Queue = ctx.device.graphics_queue;
	imgui_vk_info.DescriptorPool = m_descriptor_pool;
	imgui_vk_info.MinImageCount = 2;
	imgui_vk_info.ImageCount = 2;
	imgui_vk_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	imgui_vk_info.UseDynamicRendering = true;    // Need to explicitly enable VK_KHR_dynamic_rendering extension to use this, even for Vulkan 1.3.
	imgui_vk_info.ColorAttachmentFormat = VulkanRendererCommon::get_instance().swapchain_color_format;  // Required for dynamic rendering
	
	assert(ImGui_ImplVulkan_Init(&imgui_vk_info, VK_NULL_HANDLE));

	VkCommandBuffer cbuf = begin_temp_cmd_buffer();
	ImGui_ImplVulkan_CreateFontsTexture(cbuf);
	end_temp_cmd_buffer(cbuf);

	create_scene_viewport_attachment();

	/* Dynamic renderpass setup */
	create_renderpass();
}

void  VulkanImGuiRenderer::create_renderpass()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_renderpass[i].reset();
		render_area = { ctx.swapchain->color_attachments[i].info.width, ctx.swapchain->color_attachments[i].info.height };
		m_renderpass[i].add_color_attachment(ctx.swapchain->color_attachments[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
	}
}
void VulkanImGuiRenderer::render(VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "ImGui Pass");

	m_renderpass[ctx.curr_frame_idx].begin(cmdBuffer, render_area);
	
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

	m_renderpass[ctx.curr_frame_idx].end(cmdBuffer);
}

void VulkanImGuiRenderer::destroy()
{
	vkDeviceWaitIdle(ctx.device);
	ImGui_ImplVulkan_Shutdown();
}

const glm::vec2& VulkanImGuiRenderer::get_render_area() const
{
	return render_area;
}

void VulkanImGuiRenderer::create_scene_viewport_attachment()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		scene_viewport_attachments[i].init(ctx.swapchain->info.color_format, scene_viewport_size.x, scene_viewport_size.y, 1, false, "Scene Viewport Attachment");
		scene_viewport_attachments[i].create(ctx.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		scene_viewport_attachments[i].transition_immediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		scene_viewport_attachments_ids[i] = ImGui_ImplVulkan_AddTexture(VulkanRendererCommon::get_instance().s_SamplerClampLinear, scene_viewport_attachments[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

