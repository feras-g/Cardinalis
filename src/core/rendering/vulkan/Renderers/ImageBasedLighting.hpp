#pragma once

#include "IRenderer.h"
#include "core/rendering/vulkan/VulkanUI.h"

static std::string env_map_folder("../../../data/textures/env/");
static VkFormat env_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;

struct ImageBasedLighting : public IRenderer
{
	void init() override
	{
		load_env_map("newport_loft.hdr");
		init_ui();
	}

	void load_env_map(const char* filename)
	{
		spherical_env_map.create_from_file(env_map_folder + filename, env_map_format, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void init_ui()
	{
		VkSampler& sampler_clamp_nearest = VulkanRendererCommon::get_instance().s_SamplerClampNearest;
		spherical_env_map_ui_id = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler_clamp_nearest, spherical_env_map.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}

	void show_ui()
	{
		if (ImGui::Begin("IBL Viewer"))
		{
			ImGui::SeparatorText("Spherical Environment Map");
			ImGui::Image(spherical_env_map_ui_id, ImGui::GetContentRegionAvail());
		}
		ImGui::End();
	}

	void create_pipeline() override {}
	bool reload_pipeline() override { return false; }
	void create_renderpass() override {}
	void render(VkCommandBuffer cmd_buffer) override {}


	/* 2D environment image storing the incoming radiances Li*/
	Texture2D spherical_env_map;
	ImTextureID spherical_env_map_ui_id;
};