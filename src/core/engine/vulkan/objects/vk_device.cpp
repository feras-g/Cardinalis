#include "vk_device.h"
#include "core/engine/common.h"

namespace vk
{
	VkResult vk::device::create()
	{
		// Instance
		VkApplicationInfo application_info =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = "",
			.applicationVersion = 0,
			.pEngineName = "",
			.engineVersion = 0,
			.apiVersion = vk_api_version,
		};

		helper_funcs.enable_instance_extensions_layers(enabled_instance_layers, enabled_instance_extensions);

		VkInstanceCreateInfo instance_create_info =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &application_info,
			.enabledLayerCount = (uint32_t)enabled_instance_layers.size(),
			.ppEnabledLayerNames = enabled_instance_layers.data(),
			.enabledExtensionCount = (uint32_t)enabled_instance_extensions.size(),
			.ppEnabledExtensionNames = enabled_instance_extensions.data(),
		};

		VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance);

		// Device
		if (result == VK_SUCCESS)
		{
			// Physical device
			uint32_t physical_device_count = 1;
			result = vkEnumeratePhysicalDevices(instance, &physical_device_count, &physical_device);

			if (result == VK_SUCCESS || result == VK_INCOMPLETE)
			{
				// Physical device properties
				vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
				limits = physical_device_properties.limits;

				helper_funcs.enable_device_extensions(enabled_device_extensions);
				
				/* Enabled device features */
				VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_feature = {};
				VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {};
				VkPhysicalDeviceMultiviewFeaturesKHR multiview_feature = {};
				VkPhysicalDeviceDepthClampZeroOneFeaturesEXT depth_clamp_feature = {};
				VkPhysicalDeviceSynchronization2Features synchronization2_feature = {};
				VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamic_state3_features = {};

				/* Descriptor indexing */
				descriptor_indexing_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
				descriptor_indexing_feature.pNext = &dynamic_rendering_feature;
				descriptor_indexing_feature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
				descriptor_indexing_feature.descriptorBindingPartiallyBound = VK_TRUE;
				descriptor_indexing_feature.runtimeDescriptorArray = VK_TRUE;
				descriptor_indexing_feature.descriptorBindingVariableDescriptorCount = VK_TRUE;

				/* Dynamic rendering */
				dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
				dynamic_rendering_feature.pNext = &multiview_feature;
				dynamic_rendering_feature.dynamicRendering = VK_TRUE;

				/* Multiview */
				multiview_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
				multiview_feature.pNext = &depth_clamp_feature;
				multiview_feature.multiview = VK_TRUE;

				/* Depth clamp */
				depth_clamp_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT;
				depth_clamp_feature.pNext = &synchronization2_feature;
				depth_clamp_feature.depthClampZeroOne = VK_TRUE;

				/* Synchronization 2 */
				synchronization2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
				synchronization2_feature.pNext = VK_NULL_HANDLE;
				synchronization2_feature.synchronization2 = VK_TRUE;

				physical_device_features = {};
				physical_device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				physical_device_features.pNext = &descriptor_indexing_feature;
				vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);

				uint32_t queue_family_count;
				std::vector<VkQueueFamilyProperties> queue_family_properties;
				vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
				queue_family_properties.resize(queue_family_count);
				vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

				queue_family_indices[queue_family::graphics] = helper_funcs.get_queue_family_index(VK_QUEUE_GRAPHICS_BIT, queue_family_properties);
				queue_family_indices[queue_family::compute]  = helper_funcs.get_queue_family_index(VK_QUEUE_COMPUTE_BIT, queue_family_properties);
				queue_family_indices[queue_family::transfer] = helper_funcs.get_queue_family_index(VK_QUEUE_TRANSFER_BIT, queue_family_properties);

				VkDeviceQueueCreateInfo queues_create_info[queue_family::count] = {};
				for (int i = 0; i < queue_family::count; i++)
				{
					queues_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queues_create_info[i].pQueuePriorities = &default_queue_priority;
					queues_create_info[i].queueCount = 1u;
					queues_create_info[i].queueFamilyIndex = queue_family_indices[i];
				}

				VkDeviceCreateInfo device_create_info =
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
					.pNext = &physical_device_features,
					.flags = 0,
					.queueCreateInfoCount = queue_family::count,
					.pQueueCreateInfos = queues_create_info,
					.enabledExtensionCount = (uint32_t)enabled_device_extensions.size(),
					.ppEnabledExtensionNames = enabled_device_extensions.data(),
					.pEnabledFeatures = nullptr,
				};

				VK_CHECK((result = vkCreateDevice(physical_device, &device_create_info, nullptr, &m_device)));

				if (result == VK_SUCCESS)
				{
					vkGetDeviceQueue(m_device, queue_family_indices[queue_family::graphics], 0, &graphics_queue);
					vkGetDeviceQueue(m_device, queue_family_indices[queue_family::compute], 0, &compute_queue);
					vkGetDeviceQueue(m_device, queue_family_indices[queue_family::transfer], 0, &transfer_queue);
				}
				
				helper_funcs.load_device_function_pointers(m_device);
			}
		}

		return result;
	}

	void device::helpers::enable_instance_extensions_layers(std::vector<const char*>& enabled_instance_layers, std::vector<const char*>& enabled_instance_extensions)
	{
		enabled_instance_extensions =
		{
			"VK_KHR_surface",
	#ifdef _WIN32
			"VK_KHR_win32_surface",
	#endif // _WIN32
	#ifdef ENGINE_DEBUG
			"VK_EXT_debug_utils",
			"VK_EXT_validation_features",
	#endif // ENGINE_DEBUG
		};

		enabled_instance_layers =
		{
	#ifdef ENGINE_DEBUG
			"VK_LAYER_KHRONOS_validation",
	#endif // ENGINE_DEBUG
			"VK_LAYER_LUNARG_monitor"
		};
	}
	void device::helpers::enable_device_extensions(std::vector<const char*>& enabled_device_extensions)
	{
		enabled_device_extensions =
		{
			"VK_KHR_maintenance1",
			"VK_KHR_swapchain",
			"VK_KHR_dynamic_rendering",
			"VK_EXT_debug_marker"
		};
	}
	void device::helpers::enable_physical_device_features(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures2& physical_features2)
	{
		/* Enabled device features */
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_feature = {};
		VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {};
		VkPhysicalDeviceMultiviewFeaturesKHR multiview_feature = {};
		VkPhysicalDeviceDepthClampZeroOneFeaturesEXT depth_clamp_feature = {};
		VkPhysicalDeviceSynchronization2Features synchronization2_feature = {};
		VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamic_state3_features = {};

		/* Descriptor indexing */
		descriptor_indexing_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		descriptor_indexing_feature.pNext = &dynamic_rendering_feature;
		descriptor_indexing_feature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		descriptor_indexing_feature.descriptorBindingPartiallyBound = VK_TRUE;
		descriptor_indexing_feature.runtimeDescriptorArray = VK_TRUE;
		descriptor_indexing_feature.descriptorBindingVariableDescriptorCount = VK_TRUE;

		/* Dynamic rendering */
		dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		dynamic_rendering_feature.pNext = &multiview_feature;
		dynamic_rendering_feature.dynamicRendering = VK_TRUE;

		/* Multiview */
		multiview_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		multiview_feature.pNext = &depth_clamp_feature;
		multiview_feature.multiview = VK_TRUE;

		/* Depth clamp */
		depth_clamp_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT;
		depth_clamp_feature.pNext = &synchronization2_feature;
		depth_clamp_feature.depthClampZeroOne = VK_TRUE;

		/* Synchronization 2 */
		synchronization2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		synchronization2_feature.pNext = VK_NULL_HANDLE;
		synchronization2_feature.synchronization2 = VK_TRUE;

		physical_features2 = {};
		physical_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physical_features2.pNext = &descriptor_indexing_feature;
		vkGetPhysicalDeviceFeatures2(physical_device, &physical_features2);
	}
	int  device::helpers::get_queue_family_index(VkQueueFlagBits queue_family, std::span<VkQueueFamilyProperties> queue_family_properties)
	{
		for (int queue_index = 0; queue_index < queue_family_properties.size(); queue_index++)
		{
			VkQueueFlags queue_flags = queue_family_properties[queue_index].queueFlags;

			if (!!(queue_family & VK_QUEUE_COMPUTE_BIT))
			{
				if (!!(queue_flags & VK_QUEUE_COMPUTE_BIT) && (queue_flags & VK_QUEUE_GRAPHICS_BIT) == 0)
				{
					return queue_index;
				}
			}
			else if (!!(queue_family & VK_QUEUE_TRANSFER_BIT))
			{
				if (!!(queue_flags & VK_QUEUE_TRANSFER_BIT) && (queue_flags & VK_QUEUE_GRAPHICS_BIT) == 0 && (queue_flags & VK_QUEUE_COMPUTE_BIT) == 0)
				{
					return queue_index;
				}
			}
			else
			{
				if (!!(queue_flags & queue_family))
				{
					return queue_index;
				}
			}
		}

		LOG_ERROR("Could not find queue family index for queue family.");
		return -1;
	}
	void device::helpers::load_device_function_pointers(VkDevice device)
	{
		fpCmdBeginDebugUtilsLabelEXT = PFN_vkCmdBeginDebugUtilsLabelEXT(vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
		fpCmdEndDebugUtilsLabelEXT = PFN_vkCmdEndDebugUtilsLabelEXT(vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
		fpCmdInsertDebugUtilsLabelEXT = PFN_vkCmdInsertDebugUtilsLabelEXT(vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT"));
		fpSetDebugUtilsObjectNameEXT = PFN_vkSetDebugUtilsObjectNameEXT(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
	}

	uint32_t device::find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags memory_properties)
	{
		VkPhysicalDeviceMemoryProperties device_mem_properties;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &device_mem_properties);

		for (uint32_t i = 0; i < device_mem_properties.memoryTypeCount; i++)
		{
			if ((memory_type_bits & (1 << i) && (device_mem_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties))
			{
				return i;
			}
		}

		return 0;
	}
}

