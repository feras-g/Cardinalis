#pragma once

#include <span>

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
	operator VkDescriptorSetLayout()  { return vk_set_layout; }


	inline void add_uniform_buffer_binding(uint32_t binding, VkShaderStageFlags shaderStage, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, shaderStage });
	}

	inline void add_storage_buffer_binding(uint32_t binding, VkShaderStageFlags shaderStage, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, shaderStage });
	}

	inline void add_combined_image_sampler_binding(uint32_t binding, VkShaderStageFlags shaderStage, uint32_t count, VkSampler* sampler, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count, shaderStage, sampler });
	}

	inline void add_storage_image_binding(uint32_t binding, std::string_view name)
	{
		bindings.emplace_back(VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT });
	}

	inline void create(std::string_view name = "")
	{
		binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		binding_flags_info.bindingCount = (uint32_t)binding_flags.size();
		binding_flags_info.pBindingFlags = binding_flags.empty() ? nullptr : binding_flags.data();

		vk_set_layout = create_descriptor_set_layout(bindings, binding_flags_info);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vk_set_layout, name.data());
		is_created = true;
	}

	VkDescriptorSetLayout vk_set_layout;
	std::unordered_map<std::string, size_t> binding_name_to_index;
	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
	std::vector<VkDescriptorBindingFlags> binding_flags;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	bool is_created = false;
};

struct DescriptorSet
{
	/* Overload conversion operators */
	operator const VkDescriptorSet () const { return vk_set; }
	operator const VkDescriptorSet*() const { return &vk_set; }

	void assign_layout(DescriptorSetLayout in_layout)
	{
		layout = in_layout;
	}

	void create(VkDescriptorPool pool, std::string_view name)
	{
		vk_set = create_descriptor_set(pool, layout.vk_set_layout);
		set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vk_set, name.data());
	}

	void write_descriptor_uniform_buffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo buffer_descriptor_info = {};
		buffer_descriptor_info.buffer = buffer;
		buffer_descriptor_info.offset = offset;
		buffer_descriptor_info.range = range;

		VkWriteDescriptorSet buffer_descriptor_write = BufferWriteDescriptorSet(vk_set, binding, buffer_descriptor_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vkUpdateDescriptorSets(context.device, 1u, &buffer_descriptor_write, 0, nullptr);
	}

	void write_descriptor_storage_buffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo buffer_descriptor_info =
		{
			.buffer = buffer,
			.offset = offset,
			.range = range,
		};

		VkWriteDescriptorSet buffer_descriptor_write =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = vk_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &buffer_descriptor_info,
			.pTexelBufferView = nullptr,
		};

		vkUpdateDescriptorSets(context.device, 1u, &buffer_descriptor_write, 0, nullptr);
	}

	void write_descriptor_combined_image_sampler(uint32_t binding, VkImageView view, VkSampler sampler, uint32_t array_offset = 0, uint32_t descriptor_count = 1)
	{
		VkDescriptorImageInfo image_descriptor_info
		{
			.sampler   = sampler,
			.imageView = view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
	
		VkWriteDescriptorSet write = ImageWriteDescriptorSet(vk_set, binding, image_descriptor_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_offset, descriptor_count);
		
		vkUpdateDescriptorSets(context.device, 1u, &write, 0, nullptr);
	}

	void update(std::span<VkWriteDescriptorSet> descriptor_writes)
	{
		vkUpdateDescriptorSets(context.device, (uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	VkDescriptorSet vk_set;
	DescriptorSetLayout layout;
};

