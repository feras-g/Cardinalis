#pragma once

#include "core/engine/common.h"
#include "core/engine/vulkan/vk_common.h"

namespace vk
{
	enum queue_family { graphics, compute, transfer, count };

	class device
	{
	public:
		VkResult create();
		operator VkDevice() const { return m_device; }
		VkInstance instance;
		VkPhysicalDevice physical_device;
		VkPhysicalDeviceLimits limits = {};
	public:
		float default_queue_priority = 1.0;
		uint32_t queue_family_indices[3];
		VkQueue graphics_queue = VK_NULL_HANDLE;
		VkQueue compute_queue = VK_NULL_HANDLE;
		VkQueue transfer_queue = VK_NULL_HANDLE;
	protected:
		VkDevice m_device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties physical_device_properties = {};
		VkPhysicalDeviceFeatures2 physical_device_features = {};

		std::vector<const char*> enabled_instance_extensions;
		std::vector<const char*> enabled_instance_layers;
		std::vector<const char*> enabled_device_extensions;

		struct helpers
		{
			void enable_instance_extensions_layers(std::vector<const char*>& enabled_instance_layers, std::vector<const char*>& enabled_instance_extensions);
			void enable_device_extensions(std::vector<const char*>& enabled_device_extensions);
			void enable_physical_device_features(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures2& physical_device_features);
			int get_queue_family_index(VkQueueFlagBits queue_family, std::span<VkQueueFamilyProperties> queue_family_properties);
		} helper_funcs;

	};
}