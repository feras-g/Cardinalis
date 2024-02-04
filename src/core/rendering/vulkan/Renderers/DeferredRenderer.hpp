#pragma once

#include "DebugLineRenderer.hpp"
#include "core/rendering/Material.hpp"
#include "core/rendering/vulkan/VulkanUI.h"
#include "core/rendering/vulkan/Renderers/IBLPrefiltering.hpp"
#include "core/rendering/vulkan/Renderers/ShadowRenderer.hpp"

#include "core/rendering/lighting.h"


struct DeferredRenderer : public IRenderer
{
	static int render_size;
	static float inv_render_size;
	static VkFormat base_color_format;
	static VkFormat normal_format;
	static VkFormat metalness_roughness_format;
	static VkFormat depth_format;
	static VkFormat light_accumulation_format;

	void init();
	void create_pipeline() override;
	void create_renderpass() override;
	void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list);
	void show_ui(camera camera);
	void show_ui() override;
	bool reload_pipeline() override;

	static inline struct GBuffer
	{
		static void init();
		Texture2D base_color_attachment[NUM_FRAMES];
		Texture2D normal_attachment[NUM_FRAMES];
		Texture2D metalness_roughness_attachment[NUM_FRAMES];
		Texture2D depth_attachment[NUM_FRAMES];
		Texture2D light_accumulation_attachment[NUM_FRAMES];
		Texture2D final_lighting[NUM_FRAMES];
	} gbuffer;

	/* Render pass writing geometry information to G-Buffers */
	struct GeometryPass
	{
		void create_pipeline();
		void create_renderpass();
		void render(VkCommandBuffer cmd_buffer, std::span<size_t> mesh_list);

		Pipeline pipeline;
		vk::renderpass_dynamic renderpass[NUM_FRAMES];
		VertexFragmentShader shader;
	} geometry_pass;

	/* Compositing render pass using G-Buffers to compute lighting and render to fullscreen quad */
	struct LightingPass
	{
		void create_pipeline();
		void create_renderpass();
		void render(VkCommandBuffer cmd_buffer);

		Pipeline pipeline;
		vk::renderpass_dynamic renderpass[NUM_FRAMES];
		VertexFragmentShader shader;
		vk::descriptor_set sampled_images_descriptor_set[NUM_FRAMES];
		vk::descriptor_set_layout sampled_images_descriptor_set_layout;

		const int light_volume_type_directional = 1;
		const int light_volume_type_point = 2;
		const int light_volume_type_spot = 3;

		struct light_volume_pass_data
		{
			float inv_screen_size;
			int light_type;
		} light_volume_additional_data;

	} lighting_pass;

	void render(VkCommandBuffer cmd_buffer) {}

	static inline struct UITextureIDs
	{
		static void init();
		ImTextureID base_color[NUM_FRAMES];
		ImTextureID normal[NUM_FRAMES];
		ImTextureID metalness_roughness[NUM_FRAMES];
		ImTextureID depth[NUM_FRAMES];
		ImTextureID light_accumulation[NUM_FRAMES];
		ImTextureID final_lighting[NUM_FRAMES];
	} ui_texture_ids;



	vk::renderpass_dynamic renderpass[NUM_FRAMES];
	bool render_ok = false;

	VkDescriptorPool descriptor_pool;
	vk::descriptor_set descriptor_set;

	DrawMetricsEntry draw_metrics;
};