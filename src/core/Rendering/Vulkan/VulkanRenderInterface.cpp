#include "VulkanRenderInterface.h"

#include "Window/Window.h"

#include <vector>

constexpr VkFormat ENGINE_SWAPCHAIN_COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
constexpr VkFormat ENGINE_SWAPCHAIN_DS_FORMAT    = VK_FORMAT_D32_SFLOAT;
constexpr VkColorSpaceKHR ENGINE_SWAPCHAIN_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

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

	m_InitSuccess = true;

	LOG_INFO("Vulkan init successful.");
}

void VulkanRenderInterface::Terminate()
{
	//context.swapchain->Destroy();

	//for (uint32_t i = 0; i < NUM_FRAMES; i++)
	//{
	//	context.frames[i].Destroy(context.device);
	//}

	//vkDestroySurfaceKHR(context.instance, m_Surface, nullptr);
	//vkDestroyInstance(context.instance, nullptr);
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

	deviceExtensions = { "VK_KHR_swapchain", "VK_KHR_shader_draw_parameters" };
	VkDeviceCreateInfo deviceInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
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
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = context.gfxQueueFamily
	};

	// Create main command pool/buffer
	vkCreateCommandPool(context.device, &poolCreateInfo, nullptr, &context.mainCmdPool);
	VkCommandBufferAllocateInfo cmdBufferAllocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = context.mainCmdPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	vkAllocateCommandBuffers(context.device, &cmdBufferAllocInfo, &context.mainCmdBuffer);

	// Create a command pool and command buffer for each frame
	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		vkCreateCommandPool(context.device, &poolCreateInfo, nullptr, &context.frames[i].cmdPool);

		cmdBufferAllocInfo.commandPool = context.frames[i].cmdPool;

		vkAllocateCommandBuffers(context.device, &cmdBufferAllocInfo, &context.frames[i].cmdBuffer);
	}
}

void VulkanRenderInterface::CreateSyncStructures()
{
	VkFenceCreateInfo fenceInfo			= { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,		.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	.flags = 0 };

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		VK_CHECK(vkCreateFence(context.device, &fenceInfo, nullptr, &context.frames[i].renderFence));

		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].imageAcquiredSemaphore));
		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].renderCompleteSemaphore));
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
	return context.frames[context.frameCount % NUM_FRAMES];
}

void BeginCommandBuffer(VkCommandBuffer cmdBuffer)
{
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
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

		if (strcmp(e.extensionName, "VK_EXT_debug_utils") == 0)
		{
			extensions.push_back("VK_EXT_debug_utils");
		}
		
		// Debug markers
		if (strcmp(e.extensionName, "VK_EXT_debug_report") == 0)
		{
			extensions.push_back("VK_EXT_debug_report");
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties,VkBuffer& out_Buffer, VkDeviceMemory& out_BufferMemory)
{
	VkBufferCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VK_CHECK(vkCreateBuffer(context.device, &info, nullptr, &out_Buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.device, out_Buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = size,
		.memoryTypeIndex = FindMemoryType(context.physicalDevice, memRequirements.memoryTypeBits, memProperties)
	};
	VK_CHECK(vkAllocateMemory(context.device, &allocInfo, nullptr, &out_BufferMemory));

	VK_CHECK(vkBindBufferMemory(context.device, out_Buffer, out_BufferMemory, 0));

	return true;
}
bool UploadBufferData(const VkDeviceMemory bufferMemory, VkDeviceSize offsetInBytes, const void* data, const size_t dataSizeInBytes)
{
	void* pMappedData = nullptr;
	VK_CHECK(vkMapMemory(context.device, bufferMemory, offsetInBytes, dataSizeInBytes, 0, &pMappedData));
	memcpy(pMappedData, data, dataSizeInBytes);
	vkUnmapMemory(context.device, bufferMemory);

	return true;
}

bool CreateUniformBuffer(VkDeviceSize size, VkBuffer& out_Buffer, VkDeviceMemory& out_BufferMemory)
{
	return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,out_Buffer, out_BufferMemory);
}

bool CreateColorDepthRenderPass(const RenderPassInitInfo& rpi, bool useDepth, VkRenderPass* out_renderPass)
{
	// Attachments
	// COLOR
	VkAttachmentDescription colorAttachment =
	{
		.format			= context.swapchain->info.colorFormat,
		.samples		= VK_SAMPLE_COUNT_1_BIT,														// No multisampling
		.loadOp			= rpi.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,	// Clear when render pass begins ?
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,													// Discard content when render pass ends ?
		.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE, 
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout	= (rpi.flags & RENDERPASS_FIRST) == RENDERPASS_FIRST ? 
						VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout	= (rpi.flags & RENDERPASS_LAST) == RENDERPASS_LAST ? 
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	// DEPTH
	VkAttachmentDescription depthAttachment =
	{
		.format = context.swapchain->info.depthStencilFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,														// No multisampling
		.loadOp  = rpi.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,	// Clear when render pass begins ?
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,													// Discard content when render pass ends ?
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = (rpi.flags & RENDERPASS_FIRST) == RENDERPASS_FIRST ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		//(rpi.flags & RENDERPASS_LAST) == RENDERPASS_LAST ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthAttachmentRef = { .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

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

	// Subpass
	VkSubpassDescription subpass =
	{
		.flags=0,
		.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount=0,
		.pInputAttachments= nullptr,
		.colorAttachmentCount=1,
		.pColorAttachments=&colorAttachmentRef,
		.pResolveAttachments=nullptr,
		.pDepthStencilAttachment= useDepth ? &depthAttachmentRef : nullptr,
		.preserveAttachmentCount=0,
		.pPreserveAttachments=nullptr,
	};
	
	// Renderpass
	std::vector<VkAttachmentDescription> attachments = { colorAttachment };
	if (useDepth) attachments.push_back(depthAttachment);

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.flags = 0,
		.attachmentCount = (uint32_t)attachments.size(),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses   = &subpass,
		.dependencyCount = (uint32_t)dependencies.size(),
		.pDependencies   = dependencies.data()
	};

	return (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, out_renderPass) == VK_SUCCESS);
}

bool CreateColorDepthFramebuffers(VkRenderPass renderPass, VulkanSwapchain* swapchain, VkFramebuffer* out_Framebuffers, bool useDepth)
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
		VkImageView attachments[2] = { swapchain->images[i].view, useDepth ? swapchain->depthImages[i].view : VK_NULL_HANDLE };

		fbInfo.pAttachments = attachments;
		fbInfo.width = swapchain->images[i].info.width;
		fbInfo.height = swapchain->images[i].info.height;

		VK_CHECK(vkCreateFramebuffer(context.device, &fbInfo, nullptr, &out_Framebuffers[i]));
	}

	return true;
}

bool CreateDescriptorPool(uint32_t numStorageBuffers, uint32_t numUniformBuffers, uint32_t numCombinedSamplers, VkDescriptorPool* out_DescriptorPool)
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	if (numStorageBuffers)	 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = NUM_FRAMES * numStorageBuffers   }); 
	}
	if (numUniformBuffers)	 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = NUM_FRAMES* numUniformBuffers   }); 
	}
	if (numCombinedSamplers) 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = NUM_FRAMES * numCombinedSamplers }); 
	}

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = NUM_FRAMES,
		.poolSizeCount = (uint32_t)(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(context.device, &poolInfo, nullptr, out_DescriptorPool));

	return true;
}

bool CreateGraphicsPipeline(const VulkanShader& shader, bool useBlending, bool useDepth, VkPrimitiveTopology topology, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline* out_GraphicsPipeline, float customViewportWidth, float customViewportHeight)
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

	const uint32_t stageCount = 7;

	vertexInputState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	inputAssemblyState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = { .x = 0, .y = 0, .width = 0, .height = 0, .minDepth=0.0f, .maxDepth=1.0f };
	viewport.width  = customViewportWidth  > 0 ? customViewportWidth : context.swapchain->info.extent.width;
	viewport.height = customViewportHeight > 0 ? customViewportHeight : context.swapchain->info.extent.height;

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
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
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

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	colorBlendState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
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
		.pNext=nullptr,
		.flags=0,
		.stageCount=(uint32_t)shader.pipelineStages.size(),
		.pStages=shader.pipelineStages.data(),
		.pVertexInputState=&vertexInputState,
		.pInputAssemblyState=&inputAssemblyState,
		.pTessellationState=nullptr,
		.pViewportState=&viewportState,
		.pRasterizationState=&rasterizerState,
		.pMultisampleState=&multisampleState,
		.pDepthStencilState=useDepth ? &depthStencilState : VK_NULL_HANDLE,
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

size_t CreateIndexVertexBuffer(const void* vtxData, size_t vtxBufferSizeInBytes, const void* idxData, size_t idxBufferSizeInBytes, VkBuffer out_StorageBuffer, VkDeviceMemory out_StorageBufferMem)
{
	size_t totalSizeInBytes = vtxBufferSizeInBytes + idxBufferSizeInBytes;

	// Staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMem;

	CreateBuffer(totalSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMem);

	// Copy vertex + index data to staging buffer
	void* pData;
	vkMapMemory(context.device, stagingBufferMem, 0, totalSizeInBytes, 0, &pData);
	memcpy(pData, vtxData, idxBufferSizeInBytes);
	memcpy((unsigned char*)pData + vtxBufferSizeInBytes, idxData, idxBufferSizeInBytes);
	vkUnmapMemory(context.device, stagingBufferMem);

	// Create storage buffer containing non-interleaved vertex + index data 
	CopyBuffer(stagingBuffer, out_StorageBuffer, totalSizeInBytes);

	return totalSizeInBytes;
}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSizeInBytes)
{
	BeginCommandBuffer(context.mainCmdBuffer);

	VkBufferCopy bufferRegion =
	{
		.srcOffset = 0, .dstOffset = 0, .size = bufferSizeInBytes
	};

	vkCmdCopyBuffer(context.mainCmdBuffer, srcBuffer, dstBuffer, 1, &bufferRegion);

	EndCommandBuffer(context.mainCmdBuffer);
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

VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo* imageInfo)
{
	return VkWriteDescriptorSet
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = bindingIndex,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
