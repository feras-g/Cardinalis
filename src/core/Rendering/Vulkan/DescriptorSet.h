#pragma once

#include "VulkanDebugUtils.h"

static void example_descriptor_set_creation()
{
	/*
		Example showing the creation of a descriptor set containing
		one uniform buffer descriptor and one combined image sampler
	*/

	//VkDescriptorPool pool = create_descriptor_pool(0, 1, 1, 0, NUM_FRAMES);
	//const VkSampler& sampler = VulkanRendererBase::s_SamplerRepeatNearest;

	//DescriptorSetLayout layout;
	//layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT);
	//layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_VERTEX_BIT, sampler);
	//layout.create();

	//DescriptorSet descriptor_set;
	//descriptor_set.assign_layout(layout);
	//descriptor_set.create(pool);

	//descriptor_set.write_descriptor_uniform_buffer(0, uniform_buffer.buffer, 0, sizeof(ubo));
	//descriptor_set.write_descriptor_combined_image_sampler(1, RenderObjectManager::textures[0].view, sampler);
	//descriptor_set.update();
}

struct DescriptorSetLayout
{
	operator VkDescriptorSetLayout&() { return vk_set_layout; }

	inline void add_uniform_buffer_binding(uint32_t binding, VkShaderStageFlags shaderStage, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, shaderStage });
	}

	inline void add_storage_buffer_binding(uint32_t binding, VkShaderStageFlags shaderStage, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, shaderStage });
	}

	inline void add_combined_image_sampler_binding(uint32_t binding, VkShaderStageFlags shaderStage, VkSampler sampler, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, shaderStage, &sampler });
	}

	inline void add_storage_image_binding(uint32_t binding, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT });
	}

	inline void create(std::string_view name = "")
	{
		vk_set_layout = create_descriptor_set_layout(bindings);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vk_set_layout, name.data());
		is_created = true;
	}

	VkDescriptorSetLayout vk_set_layout;
	std::unordered_map<std::string, size_t> binding_name_to_index;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bool is_created = false;
};

struct DescriptorSet
{
	operator VkDescriptorSet&() { return vk_set; }

	inline void assign_layout(DescriptorSetLayout in_layout)
	{
		layout = in_layout;
	}

	inline void create(VkDescriptorPool pool, std::string_view name)
	{
		if (false == layout.is_created)
		{
			layout.create();
		}
		vk_set = create_descriptor_set(pool, layout.vk_set_layout);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vk_set, name.data());
	}

	inline void write_descriptor_uniform_buffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo desc_buffer_info = {};
		desc_buffer_info.buffer = buffer;
		desc_buffer_info.offset = offset;
		desc_buffer_info.range = range;

		VkWriteDescriptorSet write = BufferWriteDescriptorSet(vk_set, binding, &desc_buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptor_writes.push_back(write);
	}

	inline void write_descriptor_combined_image_sampler(uint32_t binding, VkImageView view, VkSampler sampler)
	{
		VkDescriptorImageInfo desc_img_info = {};
		desc_img_info.sampler = sampler;
		desc_img_info.imageView = view;
		desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write = ImageWriteDescriptorSet(vk_set, binding, &desc_img_info);
		descriptor_writes.push_back(write);
	}

	inline void update()
	{
		vkUpdateDescriptorSets(context.device, (uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	VkDescriptorSet vk_set;
	DescriptorSetLayout layout;

	std::vector<VkWriteDescriptorSet> descriptor_writes = {};
};