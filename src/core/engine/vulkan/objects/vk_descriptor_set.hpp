#pragma once

#include "core/engine/common.h"
#include "core/engine/vulkan/objects/vk_debug_marker.hpp"
#include "core/rendering/vulkan/VkResourceManager.h"

namespace vk
{
	static void example_descriptor_set_creation()
	{
		/*
			Example showing the creation of a descriptor set containing
			one uniform buffer descriptor and one combined image sampler
		*/

		//VkDescriptorPool pool = create_descriptor_pool(0, 1, 1, 0, NUM_FRAMES);
		//const VkSampler& sampler = VulkanRendererBase::s_SamplerRepeatNearest;

		//vk::descriptor_set_layout layout;
		//layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT);
		//layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_VERTEX_BIT, sampler);
		//layout.create();

		//vk::descriptor_set descriptor_set;
		//descriptor_set.assign_layout(layout);
		//descriptor_set.create(pool);

		//descriptor_set.write_descriptor_uniform_buffer(0, uniform_buffer.buffer, 0, sizeof(ubo));
		//descriptor_set.write_descriptor_combined_image_sampler(1, RenderObjectManager::textures[0].view, sampler);
		//descriptor_set.update();
	}


	[[nodiscard]] static VkDescriptorPool create_descriptor_pool(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets)
	{
		VkDescriptorPool out;
		VkDescriptorPoolCreateInfo poolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = 0,
			.maxSets = max_sets,
			.poolSizeCount = (uint32_t)(pool_sizes.size()),
			.pPoolSizes = pool_sizes.data()
		};

		VK_CHECK(vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &out));

		/* Add to resource manager */
		VkResourceManager::get_instance(ctx.device)->add_descriptor_pool(out);

		return out;
	}

	[[nodiscard]] static VkDescriptorSetLayout create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> layout_bindings, VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindings_flags)
	{
		bindings_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

		VkDescriptorSetLayoutCreateInfo layout_info =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &bindings_flags,
			.flags = 0,
			.bindingCount = (uint32_t)layout_bindings.size(),
			.pBindings = layout_bindings.data()
		};

		VkDescriptorSetLayout out;
		VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &layout_info, nullptr, &out));

		/* Add to resource manager */
		VkResourceManager::get_instance(ctx.device)->add_descriptor_set_layout(out);

		return out;
	}

	[[nodiscard]] static VkDescriptorSet create_descriptor_set(VkDescriptorPool pool, VkDescriptorSetLayout layout)
	{
		VkDescriptorSet out;

		VkDescriptorSetAllocateInfo allocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &layout,
		};

		VK_CHECK(vkAllocateDescriptorSets(ctx.device, &allocInfo, &out));
		return out;
	}

	struct descriptor_set_layout
	{
		operator VkDescriptorSetLayout() { return vk_set_layout; }

		enum class descriptor_type : uint8_t
		{
			uniform_buffer,
			storage_buffer,
			combined_image_sampler,
			storage_image,
			count
		};

		void add_uniform_buffer_binding(uint32_t index, VkShaderStageFlags shader_stage, std::string_view name)
		{
			add_binding(index, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,shader_stage, name);
			descriptor_pool_sizes[uint8_t(descriptor_type::uniform_buffer)].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_pool_sizes[uint8_t(descriptor_type::uniform_buffer)].descriptorCount++;
		}

		void add_storage_buffer_binding(uint32_t index, VkShaderStageFlags shader_stage, std::string_view name)
		{
			add_binding(index, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shader_stage, name);
			descriptor_pool_sizes[uint8_t(descriptor_type::storage_buffer)].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptor_pool_sizes[uint8_t(descriptor_type::storage_buffer)].descriptorCount++;
		}

		void add_combined_image_sampler_binding(uint32_t index, VkShaderStageFlags shader_stage, uint32_t count, std::string_view name)
		{
			add_binding(index, count, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shader_stage, name);
			descriptor_pool_sizes[uint8_t(descriptor_type::combined_image_sampler)].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_pool_sizes[uint8_t(descriptor_type::combined_image_sampler)].descriptorCount += count;
		}

		void add_storage_image_binding(uint32_t index, std::string_view name)
		{
			add_binding(index, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, name);
			descriptor_pool_sizes[uint8_t(descriptor_type::storage_image)].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptor_pool_sizes[uint8_t(descriptor_type::storage_image)].descriptorCount++;
		}

		void create(std::string_view name = "")
		{
			binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			binding_flags_info.bindingCount = (uint32_t)binding_flags.size();
			binding_flags_info.pBindingFlags = binding_flags.empty() ? nullptr : binding_flags.data();

			vk_set_layout = create_descriptor_set_layout(bindings, binding_flags_info);
			set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vk_set_layout, name.data());
			is_created = true;
		}

		VkDescriptorSetLayout vk_set_layout;
		std::unordered_map<std::string, VkDescriptorSetLayoutBinding&> binding_map;
		VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
		std::vector<VkDescriptorBindingFlags> binding_flags;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::array<VkDescriptorPoolSize, uint8_t(descriptor_type::count)> descriptor_pool_sizes;

		bool is_created = false;

	private:
		void add_binding(uint32_t index, uint32_t count, VkDescriptorType descriptor_type, VkShaderStageFlags shader_stage, std::string_view name)
		{
			bindings.emplace_back(VkDescriptorSetLayoutBinding{ index, descriptor_type, count, shader_stage });
			binding_map.insert({ name.data(), bindings.back() });
		}
	};

	[[nodiscard]] static VkWriteDescriptorSet write_descriptor_set_buffer(VkDescriptorSet descriptor_set, uint32_t binding, const VkDescriptorBufferInfo& desc_info, VkDescriptorType desc_type)
	{
		return VkWriteDescriptorSet
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptor_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = desc_type,
			.pImageInfo = nullptr,
			.pBufferInfo = &desc_info,
			.pTexelBufferView = nullptr,
		};
	}
	[[nodiscard]] static VkWriteDescriptorSet write_descriptor_set_image(VkDescriptorSet& descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo& imageInfo, VkDescriptorType type, uint32_t array_offset, uint32_t descriptor_count)
	{
		return VkWriteDescriptorSet
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSet,
			.dstBinding = bindingIndex,
			.dstArrayElement = array_offset,
			.descriptorCount = descriptor_count,
			.descriptorType = type,
			.pImageInfo = &imageInfo,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr,
		};
	}

	struct descriptor_set
	{
		/* Overload conversion operators */
		operator const VkDescriptorSet() const { return vk_set; }
		operator const VkDescriptorSet* () const { return &vk_set; }

		void assign_layout(descriptor_set_layout in_layout)
		{
			layout = in_layout;
		}

		void create(std::string_view name)
		{
			std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
			for (VkDescriptorPoolSize size : layout.descriptor_pool_sizes)
			{
				if (size.descriptorCount > 0)
				{
					descriptor_pool_sizes.push_back(size);
				}
			}
			vk_descriptor_pool = create_descriptor_pool(descriptor_pool_sizes, 1);
			vk_set = create_descriptor_set(vk_descriptor_pool, layout.vk_set_layout);
			set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vk_set, name.data());
		}

		void create(VkDescriptorPool descriptor_pool, std::string_view name)
		{
			vk_descriptor_pool = descriptor_pool;
			vk_set = create_descriptor_set(vk_descriptor_pool, layout.vk_set_layout);
			set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vk_set, name.data());
		}

		void write_descriptor_uniform_buffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
		{
			VkDescriptorBufferInfo buffer_descriptor_info = {};
			buffer_descriptor_info.buffer = buffer;
			buffer_descriptor_info.offset = offset;
			buffer_descriptor_info.range = range;

			VkWriteDescriptorSet buffer_descriptor_write = write_descriptor_set_buffer(vk_set, binding, buffer_descriptor_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			vkUpdateDescriptorSets(ctx.device, 1u, &buffer_descriptor_write, 0, nullptr);
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

			vkUpdateDescriptorSets(ctx.device, 1u, &buffer_descriptor_write, 0, nullptr);
		}

		void write_descriptor_combined_image_sampler(uint32_t binding, VkImageView view, VkSampler sampler, uint32_t array_offset = 0, uint32_t descriptor_count = 1)
		{
			VkDescriptorImageInfo image_descriptor_info
			{
				.sampler = sampler,
				.imageView = view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkWriteDescriptorSet write = write_descriptor_set_image(vk_set, binding, image_descriptor_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_offset, descriptor_count);

			vkUpdateDescriptorSets(ctx.device, 1u, &write, 0, nullptr);
		}

		void update(std::span<VkWriteDescriptorSet> descriptor_writes)
		{
			vkUpdateDescriptorSets(ctx.device, (uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
		}

		VkDescriptorSet vk_set;
		descriptor_set_layout layout;
		VkDescriptorPool vk_descriptor_pool;
	};

}
