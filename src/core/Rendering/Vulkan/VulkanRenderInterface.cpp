#include "VulkanRenderInterface.h"
#include "VulkanRendererBase.h"

#include "Window/Window.h"

#include <vector>

VulkanContext context;

static void GetInstanceExtensionNames(std::vector<const char*>& extensions);
static void GetInstanceLayerNames(std::vector<const char*>& layers);

VulkanRenderInterface::VulkanRenderInterface(const char* name, int maj, int min, int patch)
	: m_Name(name), minVer(min), majVer(maj), patchVer(patch), vkPhysicalDevices(1), m_InitSuccess(false)
{
}

void VulkanRenderInterface::Initialize()
{
	// Init Vulkan context
	CreateInstance();
	CreateDevices();

	// Init frames
	CreateCommandStructures();
	CreateSyncStructures();

	VulkanRendererBase::create_samplers();
	VulkanRendererBase::create_buffers();

	m_InitSuccess = true;

	LOG_INFO("Vulkan init successful.");
}

void VulkanRenderInterface::Terminate()
{
	vkDeviceWaitIdle(context.device);

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		Destroy(context.device, context.frames[i]);
	}

	vkDestroyCommandPool(context.device, context.temp_cmd_pool, nullptr);

	context.swapchain->Destroy();
	vkDestroySurfaceKHR(context.instance, m_Surface, nullptr);

	vkDestroyDevice(context.device, nullptr);
	vkDestroyInstance(context.instance, nullptr);
}

void VulkanRenderInterface::CreateInstance()
{
	uint32_t version = VK_MAKE_VERSION(majVer, minVer, patchVer);
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = m_Name,
		.applicationVersion = version,
		.pEngineName = m_Name,
		.engineVersion = version,
		.apiVersion = version,
	};

	// Define instance extensions
	GetInstanceExtensionNames(instanceExtensions);

	// Define instance layers
	GetInstanceLayerNames(instanceLayers);

	VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = NULL,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount= (uint32_t)instanceLayers.size(),
		.ppEnabledLayerNames=instanceLayers.data(),
		.enabledExtensionCount= (uint32_t)instanceExtensions.size(),
		.ppEnabledExtensionNames = instanceExtensions.data()
	};
	
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &context.instance));
	
	LOG_INFO("vkCreateInstance() : success.");
}

void VulkanRenderInterface::CreateDevices()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);
	assert(deviceCount >= 1);

	vkPhysicalDevices.resize(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &deviceCount, vkPhysicalDevices.data()));

	// Use first physical device by default
	context.physicalDevice = vkPhysicalDevices[0];

	// Enumerate devices
	LOG_INFO("Found {0} device(s) :", deviceCount);
	VkPhysicalDeviceProperties props = {};
	for (uint32_t i=0; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties(vkPhysicalDevices[i], &props);
		VulkanRenderInterface::device_limits = props.limits;
		LOG_DEBUG("{0} {1} {2}. Driver version = {3}. API Version = {4}.{5}.{6}", 
			string_VkPhysicalDeviceType(props.deviceType), props.deviceName, props.deviceID, props.driverVersion,
			VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
	}

	// Create a physical device and a queue : we chose the first
	float queuePriorities[] = {1.0f};
	VkDeviceQueueCreateInfo queueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.flags = NULL,
		.queueFamilyIndex = 0,
		.queueCount = 1,
		.pQueuePriorities = queuePriorities
	};

	deviceExtensions = { "VK_KHR_swapchain", "VK_KHR_shader_draw_parameters", "VK_EXT_descriptor_indexing", "VK_KHR_dynamic_rendering"};

	// Enable runtime descriptor indexing
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.pNext = nullptr,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE
	};

	// Enable dynamic rendering
	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
		.pNext = &indexingFeatures,
		.dynamicRendering = VK_TRUE,
	};

	VkDeviceCreateInfo deviceInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &dynamic_rendering_feature,
		.flags = NULL,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		.enabledLayerCount = (uint32_t)instanceLayers.size(),
		.ppEnabledLayerNames = instanceLayers.data(),
		.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = NULL
	};
	
	VK_CHECK(vkCreateDevice(context.physicalDevice, &deviceInfo, nullptr, &context.device));
	
	/* Initialize function pointers */
	assert(fpCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdBeginDebugUtilsLabelEXT"));
	assert(fpCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdEndDebugUtilsLabelEXT"));
	assert(fpCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(context.instance, "vkCmdInsertDebugUtilsLabelEXT"));
	assert(fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(context.instance, "vkSetDebugUtilsObjectNameEXT"));
	assert(fpCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(context.instance, "vkCmdBeginRenderingKHR"));
	assert(fpCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)(vkGetInstanceProcAddr(context.instance, "vkCmdEndRenderingKHR")));

	LOG_INFO("vkCreateDevice() : success.");
}

void VulkanRenderInterface::CreateCommandStructures()
{
	// Command queue
	vkGetDeviceQueue(context.device, context.gfxQueueFamily, 0, &context.queue);
	assert(context.queue);

	// Command buffers and command pools
	VkCommandPoolCreateInfo poolCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		//.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = context.gfxQueueFamily
	};


	VkCommandBufferAllocateInfo alloc_info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = context.temp_cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	// Create a command pool and command buffer for each frame
	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		vkCreateCommandPool(context.device, &poolCreateInfo, nullptr, &context.frames[i].cmd_pool);

		alloc_info.commandPool = context.frames[i].cmd_pool;

		vkAllocateCommandBuffers(context.device, &alloc_info, &context.frames[i].cmd_buffer);
	}
}

void VulkanRenderInterface::CreateSyncStructures()
{
	VkFenceCreateInfo fenceInfo			= { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,		.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	.flags = 0 };

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		VK_CHECK(vkCreateFence(context.device, &fenceInfo, nullptr, &context.frames[i].fence_queue_submitted));

		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].smp_image_acquired));
		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].smp_queue_submitted));
	}
}

struct WindowData
{
	HWND hWnd;
	HINSTANCE hInstance;
};

void VulkanRenderInterface::CreateSurface(Window* window)
{
#ifdef _WIN32
	assert(window->GetData()->hWnd);
	VkWin32SurfaceCreateInfoKHR  surfaceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.flags = NULL,
		.hinstance = window->GetData()->hInstance,
		.hwnd = window->GetData()->hWnd
	};

	VK_CHECK(vkCreateWin32SurfaceKHR(context.instance, &surfaceInfo, nullptr, &m_Surface));

	LOG_INFO("vkCreateWin32SurfaceKHR() success.");

#else
	assert(false);
#endif // _WIN32
}

void VulkanRenderInterface::CreateSwapchain()
{
	context.swapchain.reset(new VulkanSwapchain(m_Surface, vkPhysicalDevices[0]));
	context.swapchain->Initialize(ENGINE_SWAPCHAIN_COLOR_FORMAT, ENGINE_SWAPCHAIN_COLOR_SPACE, ENGINE_SWAPCHAIN_DS_FORMAT);
}

VulkanFrame& VulkanRenderInterface::GetCurrentFrame()
{
	return context.frames[context.frame_count % NUM_FRAMES];
}

[[nodiscard]] VkCommandBuffer begin_temp_cmd_buffer()
{
	VkCommandBuffer temp_cmd_buffer = VK_NULL_HANDLE;

	if (context.temp_cmd_pool == VK_NULL_HANDLE)
	{
		VkCommandPoolCreateInfo temp_cmd_pool_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
		};

		vkCreateCommandPool(context.device, &temp_cmd_pool_info, nullptr, &context.temp_cmd_pool);
	}

	VkCommandBufferAllocateInfo alloc_info
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = context.temp_cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VK_CHECK(vkAllocateCommandBuffers(context.device, &alloc_info, &temp_cmd_buffer));

	VkCommandBufferBeginInfo beginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(temp_cmd_buffer, &beginInfo));

	return temp_cmd_buffer;
}

void end_temp_cmd_buffer(VkCommandBuffer cmd_buffer)
{
	VK_CHECK(vkEndCommandBuffer(cmd_buffer));

	/* Submit commands and signal fence when done */
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;

	VkFenceCreateInfo submit_fence_info
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0
	};
	VkFence submit_fence;

	VK_CHECK(vkCreateFence(context.device, &submit_fence_info, nullptr, &submit_fence));
	VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, submit_fence));

	/* Destroy temporary command buffers */
	vkWaitForFences(context.device, 1, &submit_fence, VK_TRUE, OneSecondInNanoSeconds);

	vkFreeCommandBuffers(context.device, context.temp_cmd_pool, 1, &cmd_buffer);
}

void EndCommandBuffer(VkCommandBuffer cmdBuffer)
{
	VK_CHECK(vkEndCommandBuffer(cmdBuffer));
}

void GetInstanceExtensionNames(std::vector<const char*>& extensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> instanceExtensions{ extensionCount };
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());
	LOG_INFO("Found {0} Vulkan instance extensions :", extensionCount);

	// Swapchain extension
	extensions.push_back("VK_KHR_surface");
#ifdef _WIN32
	extensions.push_back("VK_KHR_win32_surface");
#endif // _WIN32

	for (const auto& e : instanceExtensions)
	{
		LOG_DEBUG("- {0}", e.extensionName);

		// Debug markers
		if (strcmp(e.extensionName, "VK_EXT_debug_utils") == 0)
		{
			extensions.push_back("VK_EXT_debug_utils");
		}

	}
}

void GetInstanceLayerNames(std::vector<const char*>& layers)
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> instanceLayers{ layerCount };
	vkEnumerateInstanceLayerProperties(&layerCount, instanceLayers.data());

	LOG_INFO("Found {0} vulkan layers :", layerCount);

	for (const auto& p : instanceLayers)
	{
		LOG_DEBUG("- {0}", p.layerName);

#ifdef ENABLE_VALIDATION_LAYERS
		if (strcmp(p.layerName, "VK_LAYER_KHRONOS_validation") == 0)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}
#endif // _DEBUG
	}

}

bool CreateColorDepthRenderPass(const RenderPassInitInfo& rpi, VkRenderPass* out_renderPass)
{
	// Attachments
	std::vector<VkAttachmentDescription> attachments;

	// COLOR
	VkAttachmentDescription color =
	{
		.format			= rpi.colorFormat,
		.samples		= VK_SAMPLE_COUNT_1_BIT,														// No multisampling
		.loadOp			= rpi.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,	// Clear when render pass begins ?
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,													// Discard content when render pass ends ?
		.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE, 
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	if (rpi.flags & RENDERPASS_FIRST) { color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; }
	if (rpi.flags & RENDERPASS_LAST)  { color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; }
	if (rpi.flags & RENDERPASS_INTERMEDIATE_OFFSCREEN)
	{
		color.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		color.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	attachments.push_back(color);
	VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	// DEPTH
	VkAttachmentReference depthAttachmentRef; 
	if (rpi.depthStencilFormat != VK_FORMAT_UNDEFINED)
	{
		VkAttachmentDescription depthAttachment =
		{
			.format = rpi.depthStencilFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,														// No multisampling
			.loadOp = rpi.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,	// Clear when render pass begins ?
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,													// Discard content when render pass ends ?
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = (rpi.flags & RENDERPASS_FIRST) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			//(rpi.flags & RENDERPASS_LAST) == RENDERPASS_LAST ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		if (rpi.flags & RENDERPASS_INTERMEDIATE_OFFSCREEN)
		{
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		
		attachments.push_back(depthAttachment);
		depthAttachmentRef = { .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	}

	// Subpasses
	std::vector<VkSubpassDescription> subpasses =
	{
		// 0th subpass
		{
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = rpi.depthStencilFormat != VK_FORMAT_UNDEFINED ? &depthAttachmentRef : nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		}
	};

	// Subpass dependencies
	std::vector<VkSubpassDependency> dependencies =
	{
		// 0th subpass
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		}
	};

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.flags = 0,
		.attachmentCount = (uint32_t)attachments.size(),
		.pAttachments	 = attachments.data(),
		.subpassCount	 = (uint32_t)subpasses.size(),
		.pSubpasses		 = subpasses.data(),
		.dependencyCount = (uint32_t)dependencies.size(),
		.pDependencies	 = dependencies.data()
	};

	return (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, out_renderPass) == VK_SUCCESS);
}

bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const VulkanSwapchain* swapchain, VkFramebuffer* out_Framebuffers, bool useDepth)
{
	return CreateColorDepthFramebuffers(renderPass, swapchain->color_attachments.data(), swapchain->depthTextures.data(), out_Framebuffers, useDepth);
}

[[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> layout_bindings)
{
	VkDescriptorSetLayoutCreateInfo layout_info =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = 0,
		.bindingCount	= (uint32_t)layout_bindings.size(),
		.pBindings		= layout_bindings.data()
	};

	VkDescriptorSetLayout out;
	VK_CHECK(vkCreateDescriptorSetLayout(context.device, &layout_info, nullptr, &out));
	return out;
}

[[nodiscard]] VkDescriptorSet create_descriptor_set(VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
	VkDescriptorSet out;

	// Allocate memory
	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool		= pool,
		.descriptorSetCount = 1,
		.pSetLayouts		= &layout,
	};

	VK_CHECK(vkAllocateDescriptorSets(context.device, &allocInfo, &out));
	return out;
}

bool CreateColorDepthFramebuffers(VkRenderPass renderPass, const Texture2D* colorAttachments, const Texture2D* depthAttachments, VkFramebuffer* out_Framebuffers, bool useDepth)
{
	VkFramebufferCreateInfo fbInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = useDepth ? 2u : 1u,
		.layers = 1,
	};

	// One framebuffer per swapchain image
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		// 2 attachments per framebuffer : COLOR + DEPTH
		VkImageView attachments[2] = { colorAttachments[i].view, depthAttachments ? depthAttachments[i].view : VK_NULL_HANDLE};

		fbInfo.pAttachments = attachments;
		fbInfo.width  = colorAttachments[i].info.width;
		fbInfo.height = colorAttachments[i].info.height;

		VK_CHECK(vkCreateFramebuffer(context.device, &fbInfo, nullptr, &out_Framebuffers[i]));
	}

	return true;
}


[[nodiscard]] VkDescriptorPool create_descriptor_pool(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets)
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

	VK_CHECK(vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &out));
	return out;
}


[[nodiscard]] VkDescriptorPool create_descriptor_pool(uint32_t num_ssbo, uint32_t num_ubo, uint32_t num_combined_img_smp, uint32_t num_dynamic_ubo, uint32_t max_sets)
{
	VkDescriptorPool out;

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (num_ssbo)	 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = num_ssbo   }); 
	}
	if (num_ubo)	 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount =  num_ubo   }); 
	}
	if (num_combined_img_smp) 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount =  num_combined_img_smp }); 
	}
	if (num_dynamic_ubo)
	{
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = num_dynamic_ubo });
	}

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = max_sets,
		.poolSizeCount = (uint32_t)(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &out));

	return out;
}

bool GfxPipeline::CreateDynamic(const VulkanShader& shader, std::span<VkFormat> colorAttachmentFormats, VkFormat depth_format, Flags flags, VkPipelineLayout pipelineLayout,
	VkPipeline* out_GraphicsPipeline, VkCullModeFlags cullMode, VkFrontFace frontFace, glm::vec2 customViewport)
{
	VkPipelineRenderingCreateInfoKHR pipeline_create{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
	pipeline_create.pNext = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount = colorAttachmentFormats.size();
	pipeline_create.pColorAttachmentFormats = colorAttachmentFormats.data();
	pipeline_create.depthAttachmentFormat = depth_format;
	//pipeline_create.stencilAttachmentFormat = VK_NULL_HANDLE;
	
	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		flags = Flags(( (int)flags | (int)Flags::ENABLE_DEPTH_STATE));
	}

	return Create(shader, colorAttachmentFormats.size(), flags, nullptr, pipelineLayout, out_GraphicsPipeline, cullMode, frontFace, &pipeline_create, customViewport);
}

bool GfxPipeline::Create(const VulkanShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline* out_GraphicsPipeline,
	VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create, glm::vec2 customViewport)
{
	// Pipeline stages
	// Vertex Input -> Input Assembly -> Viewport -> Rasterization -> Depth-Stencil -> Color Blending
	VkPipelineVertexInputStateCreateInfo vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizerState;
	VkPipelineMultisampleStateCreateInfo multisampleState;
	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	VkPipelineColorBlendStateCreateInfo colorBlendState;
	VkPipelineDynamicStateCreateInfo dynamicState;

	vertexInputState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	inputAssemblyState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = { .x = 0, .y = 0, .width = 0, .height = 0, .minDepth=0.0f, .maxDepth=1.0f };
	viewport.width  = customViewport.x > 0 ? customViewport.x : context.swapchain->info.extent.width;
	viewport.height = customViewport.y > 0 ? customViewport.y : context.swapchain->info.extent.height;

	VkRect2D scissor = { .offset = {0,0}, .extent = {.width = (uint32_t)viewport.width, .height = (uint32_t)viewport.height } };

	viewportState =
	{
		.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount=1,
		.pViewports=&viewport,
		.scissorCount=1,
		.pScissors=&scissor,
	};

	rasterizerState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = cullMode,
		.frontFace = frontFace,
		.lineWidth = 1.0f
	};

	multisampleState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f
	};

	depthStencilState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};


	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};
	for (int i = 0; i < numColorAttachments; i++)
	{
		colorBlendAttachments.push_back(
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = !!(flags & ENABLE_ALPHA_BLENDING) ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			}
		);
	}

	colorBlendState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = (uint32_t)colorBlendAttachments.size(),
		.pAttachments = colorBlendAttachments.data(),
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	VkDynamicState dynamicStateElt[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStateElt
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	VkGraphicsPipelineCreateInfo gfxPipeline =
	{
		.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext= dynamic_pipeline_create,
		.flags=0,
		.stageCount=(uint32_t)shader.pipelineStages.size(),
		.pStages=shader.pipelineStages.data(),
		.pVertexInputState= !!(flags & DISABLE_VTX_INPUT_STATE) ? VK_NULL_HANDLE  : &vertexInputState,
		.pInputAssemblyState=&inputAssemblyState,
		.pTessellationState=nullptr,
		.pViewportState=&viewportState,
		.pRasterizationState=&rasterizerState,
		.pMultisampleState=&multisampleState,
		.pDepthStencilState= !!(flags & ENABLE_DEPTH_STATE) ? &depthStencilState : VK_NULL_HANDLE,
		.pColorBlendState=&colorBlendState,
		.pDynamicState=&dynamicState,
		.layout=pipelineLayout,
		.renderPass=renderPass,
		.subpass=0,
		.basePipelineHandle=VK_NULL_HANDLE,
		.basePipelineIndex=0
	};


	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &gfxPipeline, nullptr, out_GraphicsPipeline));

	return true;
}

size_t create_vertex_index_buffer(Buffer& result, const void* vtxData, size_t vtxBufferSizeInBytes, const void* idxData, size_t idxBufferSizeInBytes)
{
	size_t totalSizeInBytes = vtxBufferSizeInBytes + idxBufferSizeInBytes;

	// Staging buffer
	Buffer stagingBuffer;
	create_staging_buffer(stagingBuffer, totalSizeInBytes);

	// Copy vertex + index data to staging buffer
	void* pData;
	vkMapMemory(context.device, stagingBuffer.memory, 0, totalSizeInBytes, 0, &pData);
	memcpy(pData, vtxData, vtxBufferSizeInBytes);
	memcpy((unsigned char*)pData + vtxBufferSizeInBytes, idxData, idxBufferSizeInBytes);
	vkUnmapMemory(context.device, stagingBuffer.memory);

	// Create storage buffer containing non-interleaved vertex + index data 
	create_storage_buffer(result, totalSizeInBytes);
	copy_buffer(stagingBuffer, result, totalSizeInBytes);

	destroy_buffer(stagingBuffer);

	return totalSizeInBytes;
}

VkWriteDescriptorSet BufferWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType descriptorType)
{
	return VkWriteDescriptorSet
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = bindingIndex,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = bufferInfo,
		.pTexelBufferView = nullptr,
	};
}

VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo* imageInfo, VkDescriptorType type)
{
	return VkWriteDescriptorSet
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = bindingIndex,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = imageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
}

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entryPoint)
{
	return VkPipelineShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr
	};
}

bool CreateTextureSampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler)
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.flags = 0,
		.magFilter = magFilter,	// VK_FILTER_LINEAR, VK_FILTER_NEAREST
		.minFilter = minFilter, // VK_FILTER_LINEAR, VK_FILTER_NEAREST
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, // VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST
		.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 100.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	return (vkCreateSampler(context.device, &samplerInfo, nullptr, &out_Sampler) == VK_SUCCESS);
}

void BeginRenderpass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount)
{
	VkRenderPassBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = renderArea,
		.clearValueCount = clearValueCount,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void EndRenderPass(VkCommandBuffer cmdBuffer)
{
	vkCmdEndRenderPass(cmdBuffer);
}

void set_viewport_scissor(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, bool invertViewportY)
{
	
	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)width,
		.height = (float)height,	// Flip viewport origin to bottom left corner 
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	if (invertViewportY)
	{
		viewport.y = height;
		viewport.height *= -1;	// Flip viewport origin to bottom left corner 
	}

	VkRect2D scissor = { .offset = {0, 0} , .extent = { width, height} };

	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descSetLayout)
{
	std::array<VkDescriptorSetLayout, 1> layout = { descSetLayout };
	return create_pipeline_layout(device, layout, 0, 0);
}

VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> desc_set_layouts)
{
	return create_pipeline_layout(device, desc_set_layouts, 0, 0);
}

VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> desc_set_layouts, uint32_t vtxConstRangeSizeInBytes, uint32_t fragConstRangeSizeInBytes)
{
	VkPipelineLayout out;

	VkPushConstantRange pushConstantRanges[2] =
	{
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size   = vtxConstRangeSizeInBytes
		},
		{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = vtxConstRangeSizeInBytes,
			.size   = fragConstRangeSizeInBytes
		}
	};
	
	uint32_t pushConstantRangeCount = (vtxConstRangeSizeInBytes > 0) + (fragConstRangeSizeInBytes > 0);

	const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = (uint32_t)desc_set_layouts.size(),
		.pSetLayouts = desc_set_layouts.data(),
		.pushConstantRangeCount = pushConstantRangeCount,
		.pPushConstantRanges = pushConstantRangeCount == 0 ? nullptr : (vtxConstRangeSizeInBytes > 0 ? pushConstantRanges : &pushConstantRanges[1])
	};

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &out));

	return out;
}