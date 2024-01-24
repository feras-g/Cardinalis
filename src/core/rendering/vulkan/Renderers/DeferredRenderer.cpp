#pragma once

#include "DeferredRenderer.hpp"

#include "core/rendering/vulkan/Renderers/VolumetricLightRenderer.hpp"

void DeferredRenderer::create_pipeline_lighting_pass()
{
	VkFormat attachment_formats[]
	{
		light_accumulation_format
	};

	VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().smp_clamp_nearest;
	VkSampler& sampler_repeat_linear = VulkanRendererCommon::get_instance().smp_repeat_linear;
	VkSampler& sampler_clamp_linear = VulkanRendererCommon::get_instance().smp_clamp_linear;
	VkSampler& sampler_repeat_nearest = VulkanRendererCommon::get_instance().smp_repeat_nearest;

	/*
		Create descriptor set bindings
	*/
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Base Color");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Normal");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(2, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Metalness Roughness");
	sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(3, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "GBuffer Depth");

	/* Add images from image-based lighting */
	if (IBLRenderer::is_initialized)
	{
		// Only use a nearest sampler to sample these image, linear introduces artifacts probably due to averaging texels
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(4, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Pre-filtered Env Map Diffuse");
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(5, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Pre-filtered Env Map Specular");
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(6, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "BRDF Integration Map");
	}

	/* Add images from Volumetric Light renderer */
	if (VolumetricLightRenderer::is_initialized)
	{
		sampled_images_descriptor_set_layout.add_combined_image_sampler_binding(7, VK_SHADER_STAGE_FRAGMENT_BIT, 1, "Volumetric Lighting");
	}

	sampled_images_descriptor_set_layout.create("GBuffer Descriptor Layout");

	/*
		Write to bindings
	*/
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		sampled_images_descriptor_set[i].assign_layout(sampled_images_descriptor_set_layout);
		sampled_images_descriptor_set[i].create("GBuffer Descriptor Set");
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(0, gbuffer[i].base_color_attachment.view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(1, gbuffer[i].normal_attachment.view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(2, gbuffer[i].metalness_roughness_attachment.view, sampler_clamp_nearest);
		sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(3, gbuffer[i].depth_attachment.view, sampler_clamp_nearest);

		if (IBLRenderer::is_initialized)
		{
			// Only use a nearest sampler to sample these image, linear introduces artifacts probably due to averaging texels
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(4, IBLRenderer::prefiltered_diffuse_env_map.view, sampler_repeat_nearest);
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(5, IBLRenderer::prefiltered_specular_env_map.view, sampler_repeat_nearest);
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(6, IBLRenderer::brdf_integration_map.view, sampler_repeat_nearest);
		}

		if (VolumetricLightRenderer::is_initialized)
		{
			sampled_images_descriptor_set[i].write_descriptor_combined_image_sampler(7, VolumetricLightRenderer::gaussian_blur_renderer.destination.view, sampler_repeat_linear);
		}
	}

	VkDescriptorSetLayout descriptor_set_layouts[] =
	{
		VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
		sampled_images_descriptor_set_layout,
		ObjectManager::get_instance().mesh_descriptor_set_layout,
		light_manager::descriptor_set_layout,
		ShadowRenderer::descriptor_set_layout,
	};


	light_volume_additional_data.inv_screen_size = 1.0 / render_size;

	lighting_pass_pipeline.layout.add_push_constant_range("Light Volume Pass View Proj", { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(glm::mat4) });
	lighting_pass_pipeline.layout.add_push_constant_range("Light Volume Pass Additional Data", { .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = sizeof(glm::mat4), .size = sizeof(light_volume_additional_data) });

	lighting_pass_pipeline.layout.create(descriptor_set_layouts);
	shader_lighting_pass.create("render_light_volume_vert.vert.spv", "deferred_lighting_pass_frag.frag.spv");
	lighting_pass_pipeline.create_graphics(shader_lighting_pass, attachment_formats, {}, Pipeline::Flags::ENABLE_ALPHA_BLENDING, lighting_pass_pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_POLYGON_MODE_FILL);
}
