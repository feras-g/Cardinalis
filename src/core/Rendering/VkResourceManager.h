#pragma once

#include <unordered_map>
#include <functional>
#include "vulkan/vulkan.hpp"

class VkResourceManager
{
public:
	static VkResourceManager* get_instance(VkDevice device);

	size_t add_buffer(VkBuffer buffer, VkDeviceMemory memory);
	size_t add_image(VkImage image, VkDeviceMemory memory);
	size_t add_image_view(VkImageView image_view);
	size_t add_shader_module(VkShaderModule module);
	size_t add_pipeline(VkPipeline pipeline);
	size_t add_pipeline_layout(VkPipelineLayout pipeline_layout);
	size_t add_descriptor_pool(VkDescriptorPool descriptor_pool);
	size_t add_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout);
	size_t add_sampler(VkSampler sampler);

	void destroy_buffer(size_t buffer_hash);
	void destroy_image(size_t buffer_hash);
	void destroy_image_view(size_t image_view_hash);
	void destroy_shader_module(size_t module_hash);
	void destroy_pipeline(size_t pipeline_hash);
	void destroy_pipeline_layout(size_t pipeline_layout_hash);
	void destroy_descriptor_pool(size_t descriptor_pool_hash);
	void destroy_descriptor_set_layout(size_t descriptor_set_layout_hash);
	void destroy_sampler(size_t sampler_hash);
	void destroy_all_resources();

protected:
	VkResourceManager(VkDevice device);
	static VkResourceManager* s_instance;

private:
	void destroy_all_buffers();
	void destroy_all_images();
	void destroy_all_image_views();
	void destroy_all_shader_modules();
	void destroy_all_pipelines();
	void destroy_all_pipeline_layouts();
	void destroy_all_descriptor_pools();
	void destroy_all_descriptor_set_layouts();
	void destroy_all_samplers();


private:
	VkDevice m_device = VK_NULL_HANDLE;
	std::unordered_map<size_t, std::pair<VkBuffer, VkDeviceMemory>> m_buffers;
	std::unordered_map<size_t, std::pair<VkImage, VkDeviceMemory>> m_images;
	std::unordered_map<size_t, VkImageView> m_image_views;
	std::unordered_map<size_t, VkShaderModule> m_shader_modules;
	std::unordered_map<size_t, VkPipeline> m_pipelines;
	std::unordered_map<size_t, VkPipelineLayout> m_pipeline_layouts;

	std::unordered_map<size_t, VkSampler> m_samplers;

	std::unordered_map<size_t, VkDescriptorPool> m_descriptor_pools;
	std::unordered_map<size_t, VkDescriptorSet> m_descriptor_sets;
	std::unordered_map<size_t, VkDescriptorSetLayout> m_descriptor_set_layouts;
};

/* Add a vk object to the map */
template<typename T, typename U>
size_t add(T vk_object, U& container)
{
	size_t hash = std::hash<T>{}(vk_object);
	container.insert({ hash, vk_object });
	return hash;
}


