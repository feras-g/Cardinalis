#include "VulkanImGuiRenderer.h"
#include "core/rendering/vulkan/VulkanDebugUtils.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanShader.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING

void VulkanImGuiRenderer::init()
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}, 
	};

	m_descriptor_pool = create_descriptor_pool(pool_sizes, 64);

	ImGui_ImplVulkan_InitInfo imgui_vk_info = {};

	imgui_vk_info.Instance = context.instance;
	imgui_vk_info.PhysicalDevice = context.physical_device;
	imgui_vk_info.Device = context.device;
	imgui_vk_info.QueueFamily = context.gfxQueueFamily;
	imgui_vk_info.Queue = context.queue;
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

	/* Dynamic renderpass setup */
	create_renderpass();
}

void  VulkanImGuiRenderer::create_renderpass()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_renderpass[i].reset();
		render_area = { context.swapchain->color_attachments[i].info.width, context.swapchain->color_attachments[i].info.height };
		m_renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
	}
}
void VulkanImGuiRenderer::render(VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "ImGui Pass");

	m_renderpass[context.curr_frame_idx].begin(cmdBuffer, render_area);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

	m_renderpass[context.curr_frame_idx].end(cmdBuffer);
}

void VulkanImGuiRenderer::destroy()
{
	vkDeviceWaitIdle(context.device);
	ImGui_ImplVulkan_Shutdown();
}

const glm::vec2& VulkanImGuiRenderer::get_render_area() const
{
	return render_area;
}
