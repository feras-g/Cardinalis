#pragma once

#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/engine/vulkan/objects/vk_renderpass.h"
#include "core/rendering/vulkan/VulkanShader.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"
#include "../imgui/widgets/imguizmo/ImGuizmo.h"

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	void init();
	void render(VkCommandBuffer cmd_buffer);
	void create_renderpass();
	void destroy();
	const glm::vec2& get_render_area() const;

	static inline glm::uvec2 scene_viewport_size { 2048, 2048 };
	void create_scene_viewport_attachment();
	static inline Texture2D scene_viewport_attachments[NUM_FRAMES];
	static inline VkDescriptorSet scene_viewport_attachments_ids[NUM_FRAMES];
private:
	glm::vec2 render_area;
	VkDescriptorPool m_descriptor_pool;
	vk::renderpass_dynamic m_renderpass[NUM_FRAMES];
};
