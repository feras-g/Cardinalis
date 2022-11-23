#include "VulkanRenderInterface.h"

#include "Window/Window.h"

#include <vector>

#define PROJECT_APP_NAME "BeamEngine"
#define PROJECT_ENGINE_NAME "BeamEngine"
#define MAJ_VER 1
#define MIN_VER 2
#define PAT_VER 196 

#define ENGINE_SWAPCHAIN_FORMAT		 VK_FORMAT_B8G8R8A8_SRGB  
#define ENGINE_SWAPCHAIN_COLOR_SPACE VK_COLOR_SPACE_SRGB_NONLINEAR_KHR 

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
	swapchain->Destroy();

	vkDestroySurfaceKHR(context.instance, m_Surface, nullptr);
	vkDestroyInstance(context.instance, nullptr);

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		context.frames[i].Destroy(context.device);
	}
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

	deviceExtensions = { "VK_KHR_swapchain" };
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
	
	VK_CHECK(vkCreateDevice(vkPhysicalDevices[0], &deviceInfo, nullptr, &context.device));

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
	swapchain.reset(new VulkanSwapchain(m_Surface, context.instance, context.device, vkPhysicalDevices[0]));
	swapchain->Initialize(ENGINE_SWAPCHAIN_FORMAT, ENGINE_SWAPCHAIN_COLOR_SPACE);
}

VkRenderPass VulkanRenderInterface::CreateExampleRenderPass()
{
	VkRenderPass defaultRenderPass = VK_NULL_HANDLE;

	// Describe the attachments that will be used in the subpasses
	VkAttachmentDescription colorAttachment =
	{
		.format = swapchain->metadata.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,		 // No multisampling
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,  // Clear attachment when render pass begins
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Attachment content is not discarded when the render pass ends
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Attachment content may be discarded when the render pass ends
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	// Link an Id to a specific attachment
	VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	// Create subpasses : minimum is 1.
	VkSubpassDescription subpass = 
	{ 
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, 
		.colorAttachmentCount = 1, 
		.pColorAttachments = &colorAttachmentRef 
	};

	// Finally, create a renderpass incorporating all the subpasses
	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass
	};

	VK_CHECK(vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &defaultRenderPass));
	

	return defaultRenderPass;
}


void VulkanRenderInterface::CreateFramebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain)
{
	VkFramebufferCreateInfo fbInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = 1,
		.width  = swapchain.metadata.extent.width,
		.height = swapchain.metadata.extent.height,
		.layers = 1
	};

	// 1 framebuffer per swapchain image view
	for (int i = 0; i < swapchain.metadata.imageCount; i++)
	{
		fbInfo.pAttachments = &swapchain.images[i].view;
		VK_CHECK(vkCreateFramebuffer(context.device, &fbInfo, nullptr, &swapchain.framebuffers[i]));
	}
}

void VulkanRenderInterface::CreateFramebuffers(VkRenderPass renderPass)
{
	CreateFramebuffers(renderPass, *swapchain.get());
}

VulkanFrame& VulkanRenderInterface::GetCurrentFrame()
{
	return context.frames[context.frameNumber % NUM_FRAMES];
}

void BeginRecording(VkCommandBuffer cmdBuffer)
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

void EndRecording(VkCommandBuffer cmdBuffer)
{
	VK_CHECK(vkEndCommandBuffer(cmdBuffer));
}

void GetInstanceExtensionNames(std::vector<const char*>& extensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> instanceExtensions{ extensionCount };
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

	// Swapchain 
	extensions.push_back("VK_KHR_surface");
#ifdef _WIN32
	extensions.push_back("VK_KHR_win32_surface");
#endif // _WIN32

	LOG_INFO("Found {0} vulkan extensions :", extensionCount);

	for (const auto& e : instanceExtensions)
	{
		LOG_DEBUG("- {0}", e.extensionName);

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
