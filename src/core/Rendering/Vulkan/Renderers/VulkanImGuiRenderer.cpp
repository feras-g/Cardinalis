#include "VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/Renderers/ShadowRenderer.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING

VulkanImGuiRenderer::VulkanImGuiRenderer(const VulkanContext& vkContext)
{
}

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
	imgui_vk_info.ColorAttachmentFormat = VulkanRendererBase::swapchain_color_format;  // Required for dynamic rendering

	assert(ImGui_ImplVulkan_Init(&imgui_vk_info, VK_NULL_HANDLE));

	VkCommandBuffer cbuf = begin_temp_cmd_buffer();
	ImGui_ImplVulkan_CreateFontsTexture(cbuf);
	end_temp_cmd_buffer(cbuf);

	/* Add textures that will be used by ImGui */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_DeferredRendererOutputTextureId[frame_idx] = ImGui_ImplVulkan_AddTexture(VulkanRendererBase::s_SamplerClampNearest, VulkanRendererBase::m_deferred_lighting_attachment[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_ModelRendererColorTextureId[frame_idx] = ImGui_ImplVulkan_AddTexture(VulkanRendererBase::s_SamplerClampLinear, VulkanRendererBase::m_gbuffer_albedo[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_ModelRendererNormalTextureId[frame_idx] = ImGui_ImplVulkan_AddTexture(VulkanRendererBase::s_SamplerClampLinear, VulkanRendererBase::m_gbuffer_normal[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_ModelRendererDepthTextureId[frame_idx] = ImGui_ImplVulkan_AddTexture(VulkanRendererBase::s_SamplerClampLinear, VulkanRendererBase::m_gbuffer_depth[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_ModelRendererMetallicRoughnessTextureId[frame_idx] = ImGui_ImplVulkan_AddTexture(VulkanRendererBase::s_SamplerClampLinear, VulkanRendererBase::m_gbuffer_metallic_roughness[frame_idx].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	/* Dynamic renderpass setup */
	update_render_pass_attachments();
}

void  VulkanImGuiRenderer::update_render_pass_attachments()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].color_attachments.clear();
		m_dyn_renderpass[i].has_depth_attachment = false;
		m_dyn_renderpass[i].has_stencil_attachment = false;

		m_dyn_renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view);
	}
}
void VulkanImGuiRenderer::render(size_t currentImageIdx, VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "ImGui");
	
	const uint32_t width  = context.swapchain->info.extent.width;
	const uint32_t height = context.swapchain->info.extent.height;
	VkRect2D renderArea{ .offset {0, 0}, .extent { width, height } };

	m_dyn_renderpass[currentImageIdx].begin(cmdBuffer, renderArea);

	m_pDrawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(m_pDrawData, cmdBuffer);

	m_dyn_renderpass[currentImageIdx].end(cmdBuffer);
}

void VulkanImGuiRenderer::destroy()
{
	ImGui_ImplVulkan_Shutdown();
}
