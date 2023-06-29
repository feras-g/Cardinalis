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

	inline void create(std::string_view name)
	{
		layout = create_descriptor_set_layout(bindings);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)layout, name.data());
	}

	VkDescriptorSetLayout layout;
	std::unordered_map<std::string, size_t> binding_name_to_index;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct DescriptorSet
{
	void assign_layout(DescriptorSetLayout in_layout)
	{
		layout = in_layout;
	}

	void create(VkDescriptorPool pool, std::string_view name)
	{
		set = create_descriptor_set(pool, layout.layout);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)set, name.data());
	}

	void write_descriptor_uniform_buffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo desc_buffer_info = {};
		desc_buffer_info.buffer = buffer;
		desc_buffer_info.offset = offset;
		desc_buffer_info.range = range;

		VkWriteDescriptorSet write = BufferWriteDescriptorSet(set, binding, &desc_buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptor_writes.push_back(write);
	}

	void write_descriptor_combined_image_sampler(uint32_t binding, VkImageView& view, VkSampler sampler)
	{
		VkDescriptorImageInfo desc_img_info = {};
		desc_img_info.sampler = sampler;
		desc_img_info.imageView = view;
		desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write = ImageWriteDescriptorSet(set, binding, &desc_img_info);
		descriptor_writes.push_back(write);
	}

	void update()
	{
		vkUpdateDescriptorSets(context.device, (uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	VkDescriptorSet set;
	DescriptorSetLayout layout;

	std::vector<VkWriteDescriptorSet> descriptor_writes = {};
};