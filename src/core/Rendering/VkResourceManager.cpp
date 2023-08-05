#include "VkResourceManager.h"
#include "Core/EngineLogger.h"

VkResourceManager* VkResourceManager::s_instance;

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

VkResourceManager::VkResourceManager(VkDevice device) 
	: m_device(device) {}

VkResourceManager* VkResourceManager::get_instance(VkDevice device)
{
	if (nullptr == s_instance)
	{
		s_instance = new VkResourceManager(device);
	}

	return s_instance;
}

size_t VkResourceManager::add_buffer(VkBuffer buffer, VkDeviceMemory memory)
{
	size_t hash = std::hash<VkBuffer>{}(buffer);
	hash_combine<VkDeviceMemory>(hash, memory);
	m_buffers.insert({ hash , { buffer, memory } });

	LOG_INFO("Created buffer. Total buffers: {}", m_buffers.size());

	return hash;
}

size_t VkResourceManager::add_image(VkImage image, VkDeviceMemory memory)
{
	size_t hash = std::hash<VkImage>{}(image);
	hash_combine<VkDeviceMemory>(hash, memory);
	m_images.insert({ hash, { image, memory } });
	LOG_INFO("Created image. Total images: {}", m_images.size());
	return hash;
}

size_t VkResourceManager::add_image_view(VkImageView image_view)
{
	size_t hash = add(image_view, m_image_views);
	LOG_INFO("Created image view. Total image views: {}", m_image_views.size());
	return hash;
}

size_t VkResourceManager::add_shader_module(VkShaderModule module)
{
	size_t hash = add(module, m_shader_modules);
	LOG_INFO("Created shader module. Total shader modules: {}", m_shader_modules.size());
	return hash;
}

size_t VkResourceManager::add_pipeline(VkPipeline pipeline)
{
	size_t hash = add(pipeline, m_pipelines);
	LOG_INFO("Created pipeline. Total pipelines: {}", m_pipelines.size());
	return hash;
}

size_t VkResourceManager::add_pipeline_layout(VkPipelineLayout pipeline_layout)
{
	size_t hash = add(pipeline_layout, m_pipeline_layouts);
	LOG_INFO("Created pipeline layout. Total pipeline layouts: {}", m_pipeline_layouts.size());
	return hash;
}

size_t VkResourceManager::add_descriptor_pool(VkDescriptorPool descriptor_pool)
{
	size_t hash = add(descriptor_pool, m_descriptor_pools);
	LOG_INFO("Created descriptor pool. Total descriptor pools: {}", m_descriptor_pools.size());
	return hash;
}

size_t VkResourceManager::add_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout)
{
	size_t hash = add(descriptor_set_layout, m_descriptor_set_layouts);
	LOG_INFO("Created descriptor set layout. Total descriptor set layouts: {}", m_descriptor_set_layouts.size());
	return hash;
}

size_t VkResourceManager::add_sampler(VkSampler sampler)
{
	size_t hash = add(sampler, m_samplers);
	LOG_INFO("Created sampler. Total samplers: {}", m_samplers.size());
	return hash;
}

void VkResourceManager::destroy_buffer(size_t buffer_hash)
{
	auto ite = m_buffers.find(buffer_hash);
	if (ite != m_buffers.end())
	{
		if (VK_NULL_HANDLE != ite->second.first)
		{
			vkDestroyBuffer(m_device, ite->second.first, nullptr);
			ite->second.first = VK_NULL_HANDLE;
		}

		if (VK_NULL_HANDLE != ite->second.second)
		{
			vkFreeMemory(m_device, ite->second.second, nullptr);
			ite->second.second = VK_NULL_HANDLE;
		}
	}

	LOG_INFO("Destroyed buffer. Total buffers: {}", m_buffers.size());
}

void VkResourceManager::destroy_image(size_t image_hash)
{
	auto ite = m_images.find(image_hash);
	if (ite != m_images.end())
	{
		VkImage& image = ite->second.first;
		if (VK_NULL_HANDLE != image)
		{
			vkDestroyImage(m_device, image, nullptr);
			image = VK_NULL_HANDLE;
		}

		VkDeviceMemory& memory = ite->second.second;
		if (VK_NULL_HANDLE != memory)
		{
			vkFreeMemory(m_device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}
}

void VkResourceManager::destroy_image_view(size_t image_view_hash)
{
	auto ite = m_image_views.find(image_view_hash);
	if (ite != m_image_views.end())
	{
		VkImageView& image_view = ite->second;
		if (VK_NULL_HANDLE != image_view)
		{
			vkDestroyImageView(m_device, image_view, nullptr);
			image_view = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed image view. Total image views: {}", m_image_views.size());
}

void VkResourceManager::destroy_shader_module(size_t module_hash)
{
	auto ite = m_shader_modules.find(module_hash);
	if (ite != m_shader_modules.end())
	{
		VkShaderModule& module = ite->second;
		if (VK_NULL_HANDLE != module)
		{
			vkDestroyShaderModule(m_device, module, nullptr);
			module = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed shader module. Total shader modules: {}", m_shader_modules.size());
}

void VkResourceManager::destroy_pipeline(size_t pipeline_hash)
{
	auto ite = m_pipelines.find(pipeline_hash);
	if (ite != m_pipelines.end())
	{
		VkPipeline& pipeline = ite->second;
		if (VK_NULL_HANDLE != pipeline)
		{
			vkDestroyPipeline(m_device, pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed pipeline. Total pipelines: {}", m_pipelines.size());
}

void VkResourceManager::destroy_pipeline_layout(size_t pipeline_layout_hash)
{
	auto ite = m_pipeline_layouts.find(pipeline_layout_hash);
	if (ite != m_pipeline_layouts.end())
	{
		VkPipelineLayout& pipeline_layout = ite->second;
		if (VK_NULL_HANDLE != pipeline_layout)
		{
			vkDestroyPipelineLayout(m_device, pipeline_layout, nullptr);
			pipeline_layout = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed pipeline layout. Total pipeline layouts: {}", m_pipeline_layouts.size());
}

void VkResourceManager::destroy_descriptor_pool(size_t descriptor_pool_hash)
{
	auto ite = m_descriptor_pools.find(descriptor_pool_hash);
	if (ite != m_descriptor_pools.end())
	{
		VkDescriptorPool& descriptor_pool = ite->second;
		if (VK_NULL_HANDLE != descriptor_pool)
		{
			vkDestroyDescriptorPool(m_device, descriptor_pool, nullptr);
			descriptor_pool = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed descriptor pool. Total descriptor pools: {}", m_descriptor_pools.size());
}

void VkResourceManager::destroy_descriptor_set_layout(size_t descriptor_set_layout_hash)
{
	auto ite = m_descriptor_set_layouts.find(descriptor_set_layout_hash);
	if (ite != m_descriptor_set_layouts.end())
	{
		VkDescriptorSetLayout& descriptor_set_layout = ite->second;
		if (VK_NULL_HANDLE != descriptor_set_layout)
		{
			vkDestroyDescriptorSetLayout(m_device, descriptor_set_layout, nullptr);
			descriptor_set_layout = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed descriptor set layout. Total descriptor set layout: {}", m_descriptor_set_layouts.size());
}

void VkResourceManager::destroy_sampler(size_t sampler_hash)
{
	auto ite = m_samplers.find(sampler_hash);
	if (ite != m_samplers.end())
	{
		VkSampler& sampler = ite->second;
		if (VK_NULL_HANDLE != sampler)
		{
			vkDestroySampler(m_device, sampler, nullptr);
			sampler = VK_NULL_HANDLE;
		}
	}
	LOG_INFO("Destroyed sampler. Total samplers: {}", m_samplers.size());
}

void VkResourceManager::destroy_all_resources()
{
	destroy_all_buffers();
	destroy_all_images();
	destroy_all_image_views();
	destroy_all_shader_modules();
	destroy_all_pipelines();
	destroy_all_pipeline_layouts();
	destroy_all_descriptor_pools();
	destroy_all_descriptor_set_layouts();
	destroy_all_samplers();
}


void VkResourceManager::destroy_all_buffers()
{
	for (auto it = m_buffers.begin(); it != m_buffers.end();) 
	{
		destroy_buffer(it->first);
		it = m_buffers.erase(it);
	}
}

void VkResourceManager::destroy_all_images()
{
	for (auto it = m_images.begin(); it != m_images.end();)
	{
		destroy_image(it->first);
		it = m_images.erase(it);
	}
}

void VkResourceManager::destroy_all_image_views()
{
	for (auto it = m_image_views.begin(); it != m_image_views.end();)
	{
		destroy_image_view(it->first);
		it = m_image_views.erase(it);
	}
}

void VkResourceManager::destroy_all_shader_modules()
{
	for (auto it = m_shader_modules.begin(); it != m_shader_modules.end();)
	{
		destroy_shader_module(it->first);
		it = m_shader_modules.erase(it);
	}
}

void VkResourceManager::destroy_all_pipelines()
{
	for (auto it = m_pipelines.begin(); it != m_pipelines.end();)
	{
		destroy_pipeline(it->first);
		it = m_pipelines.erase(it);
	}
}

void VkResourceManager::destroy_all_pipeline_layouts()
{
	for (auto it = m_pipeline_layouts.begin(); it != m_pipeline_layouts.end();)
	{
		destroy_pipeline_layout(it->first);
		it = m_pipeline_layouts.erase(it);
	}
}

void VkResourceManager::destroy_all_descriptor_pools()
{
	for (auto it = m_descriptor_pools.begin(); it != m_descriptor_pools.end();)
	{
		destroy_descriptor_pool(it->first);
		it = m_descriptor_pools.erase(it);
	}
}

void VkResourceManager::destroy_all_descriptor_set_layouts()
{
	for (auto it = m_descriptor_set_layouts.begin(); it != m_descriptor_set_layouts.end();)
	{
		destroy_descriptor_set_layout(it->first);
		it = m_descriptor_set_layouts.erase(it);
	}
}

void VkResourceManager::destroy_all_samplers()
{
	for (auto it = m_samplers.begin(); it != m_samplers.end();)
	{
		destroy_sampler(it->first);
		it = m_samplers.erase(it);
	}
}

