#pragma once

#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/RenderPass.h"
#include "core/rendering/vulkan/VulkanShader.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	void init();
	void render(VkCommandBuffer cmd_buffer);
	void create_renderpass();
	void destroy();
	const glm::vec2& get_render_area() const;

private:
	glm::vec2 render_area;
	VkDescriptorPool m_descriptor_pool;
	VulkanRenderPassDynamic m_renderpass[NUM_FRAMES];
};
