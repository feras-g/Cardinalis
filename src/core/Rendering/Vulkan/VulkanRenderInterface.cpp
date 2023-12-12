#include "VulkanRenderInterface.h"
#include "VulkanRendererBase.h"
#include "core/rendering/vulkan/VkResourceManager.h"

#include "core/engine/Window.h"

#include <vector>

VulkanContext context;

static void GetInstanceExtensionNames(std::vector<const char*>& extensions);
static void GetInstanceLayerNames(std::vector<const char*>& layers);

RenderInterface::RenderInterface(const char* name, int maj, int min, int patch)
	: m_name(name), min_ver(min), maj_ver(maj), patch_ver(patch), m_init_success(false)
{
}

void RenderInterface::init()
{
	// Init Vulkan context
	create_device();

	// Init frames
	create_command_structures();
	create_synchronization_structures();

	m_init_success = true;

	LOG_INFO("Vulkan init successful.");
}

void RenderInterface::terminate()
{
	vkDeviceWaitIdle(context.device);
	VkResourceManager::get_instance(context.device)->destroy_all_resources();

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		destroy(context.device, context.frames[i]);
	}

	vkDestroyCommandPool(context.device, context.temp_cmd_pool, nullptr);

	context.swapchain->destroy();
	vkDestroySurfaceKHR(context.device.instance, surface, nullptr);

	vkDestroyDevice(context.device, nullptr);
	vkDestroyInstance(context.device.instance, nullptr);
}

void RenderInterface::create_device()
{
	context.device.create();
}

void RenderInterface::create_command_structures()
{
	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		context.frames[i].cmd_buffer.init(context.device, context.device.queue_family_indices[vk::queue_family::graphics]);
		context.frames[i].cmd_buffer.create(context.device);
	}
}

void RenderInterface::create_synchronization_structures()
{
	VkFenceCreateInfo fenceInfo			= { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,		.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	.flags = 0 };

	for (uint32_t i = 0; i < NUM_FRAMES; i++)
	{
		VK_CHECK(vkCreateFence(context.device, &fenceInfo, nullptr, &context.frames[i].fence_queue_submitted));

		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].semaphore_swapchain_acquire));
		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &context.frames[i].smp_queue_submitted));
	}
}

struct WindowData
{
	HWND hWnd;
	HINSTANCE hInstance;
};

void RenderInterface::create_surface(Window* window)
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

	VK_CHECK(vkCreateWin32SurfaceKHR(context.device.instance, &surfaceInfo, nullptr, &surface));

	LOG_INFO("vkCreateWin32SurfaceKHR() success.");

#else
	assert(false);
#endif // _WIN32
}

void RenderInterface::create_swapchain()
{
	context.swapchain.reset(new VulkanSwapchain(surface));
	context.swapchain->init(VulkanRendererCommon::get_instance().swapchain_color_format, 
	                        VulkanRendererCommon::get_instance().swapchain_colorspace, 
	                        VulkanRendererCommon::get_instance().swapchain_depth_format);
}

vk::frame& RenderInterface::get_current_frame()
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
	VK_CHECK(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, submit_fence));

	/* Destroy temporary command buffers */
	vkWaitForFences(context.device, 1, &submit_fence, VK_TRUE, 1000000000ull);

	vkFreeCommandBuffers(context.device, context.temp_cmd_pool, 1, &cmd_buffer);
	vkDestroyFence(context.device, submit_fence, nullptr);
}

void EndCommandBuffer(VkCommandBuffer cmdBuffer)
{
	VK_CHECK(vkEndCommandBuffer(cmdBuffer));
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
	return CreateColorDepthFramebuffers(renderPass, swapchain->color_attachments.data(), swapchain->depth_attachments.data(), out_Framebuffers, useDepth);
}

[[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> layout_bindings, VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindings_flags)
{
	bindings_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;

	VkDescriptorSetLayoutCreateInfo layout_info =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &bindings_flags,
		.flags = 0,
		.bindingCount	= (uint32_t)layout_bindings.size(),
		.pBindings		= layout_bindings.data()
	};

	VkDescriptorSetLayout out;
	VK_CHECK(vkCreateDescriptorSetLayout(context.device, &layout_info, nullptr, &out));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_descriptor_set_layout(out);

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

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_descriptor_pool(out);

	return out;
}


[[nodiscard]] VkDescriptorPool create_descriptor_pool(VkDescriptorPoolCreateFlags flags, uint32_t num_ssbo, uint32_t num_ubo, uint32_t num_combined_img_smp, uint32_t num_dynamic_ubo, uint32_t max_sets, uint32_t num_storage_image)
{
	VkDescriptorPool out;

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (num_ssbo)	 
	{ 
		poolSizes.push_back( VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = num_ssbo }); 
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
	if (num_storage_image)
	{
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = num_storage_image });
	}

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = flags,
		.maxSets = max_sets,
		.poolSizeCount = (uint32_t)(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &out));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_descriptor_pool(out);

	return out;
}

void Pipeline::create_graphics(const VertexFragmentShader& shader, std::span<VkFormat> color_formats, VkFormat depth_format, Flags flags, VkPipelineLayout pipeline_layout, VkPrimitiveTopology topology,
	VkCullModeFlags cull_mode, VkFrontFace front_face, uint32_t view_mask)
{
	for (VkFormat format : color_formats)
	{
		color_attachment_formats.push_back(format);
	}
	depth_attachment_format = depth_format;

	dynamic_pipeline_create_info = {};
	dynamic_pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	dynamic_pipeline_create_info.pNext = VK_NULL_HANDLE;
	dynamic_pipeline_create_info.colorAttachmentCount = (uint32_t)color_attachment_formats.size();
	dynamic_pipeline_create_info.pColorAttachmentFormats = color_attachment_formats.data();
	dynamic_pipeline_create_info.depthAttachmentFormat = depth_attachment_format;
	dynamic_pipeline_create_info.viewMask = view_mask;
	//pipeline_create.stencilAttachmentFormat = VK_NULL_HANDLE;
	
	pipeline_flags = flags;
	if (depth_format != VK_FORMAT_UNDEFINED)
	{
		pipeline_flags = Flags(( (int)pipeline_flags | (int)Flags::ENABLE_DEPTH_STATE));
	}

	create_graphics(shader, (uint32_t)color_formats.size(), pipeline_flags, nullptr, pipeline_layout, topology, cull_mode, front_face, &dynamic_pipeline_create_info);
}

void Pipeline::create_graphics(const VertexFragmentShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPrimitiveTopology topology,
	VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create)
{
	is_graphics = true;

	// Pipeline stages
	VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

	if (topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
	{
		polygonMode = VK_POLYGON_MODE_POINT;
	}
	else if((topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST) || (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) || (topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY) || (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY))
	{
		polygonMode = VK_POLYGON_MODE_LINE;
	}

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

	VkRect2D scissor = { .offset = {0,0}, .extent = {.width = (uint32_t)viewport.width, .height = (uint32_t)viewport.height } };

	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	raster_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_state_info.polygonMode = polygonMode;
	raster_state_info.cullMode = cullMode;
	raster_state_info.frontFace = frontFace;
	raster_state_info.lineWidth = 1.0f;
	raster_state_info.depthClampEnable = VK_TRUE;

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
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,// VK_COMPARE_OP_LESS,
	};


	for (uint32_t i = 0; i < numColorAttachments; i++)
	{
		colorBlendAttachments.push_back(
			{
				.blendEnable = !!(flags & ENABLE_ALPHA_BLENDING),
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
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
		.blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f }
	};

	dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)pipeline_dynamic_states.size(),
		.pDynamicStates = pipeline_dynamic_states.data()
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/* Save shader */
	pipeline_shader = shader;

	pipeline_create_info =
	{
		.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext= dynamic_pipeline_create,
		.flags=0,
		.stageCount=(uint32_t)pipeline_shader.stages.size(),
		.pStages= pipeline_shader.stages.data(),
		.pVertexInputState= !!(flags & DISABLE_VTX_INPUT_STATE) ? VK_NULL_HANDLE  : &vertexInputState,
		.pInputAssemblyState=&inputAssemblyState,
		.pTessellationState=nullptr,
		.pViewportState=&viewportState,
		.pRasterizationState=&raster_state_info,
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

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_pipeline(pipeline);
}

void Pipeline::create_compute(const Shader& shader, std::span<VkDescriptorSetLayout> descriptor_set_layout)
{
	layout.create(descriptor_set_layout);

	VkComputePipelineCreateInfo compute_ppl_info = {};
	compute_ppl_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_ppl_info.stage = shader.stages[0];
	compute_ppl_info.layout = layout;

	VK_CHECK(vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &compute_ppl_info, nullptr, &pipeline));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_pipeline(pipeline);
}

bool Pipeline::reload_pipeline()
{
	VkPipeline ppl;
	VkResult result;

	if (!pipeline_shader.recreate_modules())
	{
		return false;
	}

	result = vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ppl);
	if (result == VK_SUCCESS)
	{
		pipeline = ppl;
		return true;
	}
	else
	{
		LOG_ERROR("Pipeline reload failed with error : {}", string_VkResult(result));
		return false;
	}
}

void Pipeline::bind(VkCommandBuffer cmd_buffer) const
{
	VkPipelineBindPoint bind_point = is_graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(cmd_buffer, bind_point, pipeline);
}

size_t create_vertex_index_buffer(Buffer& result, const void* vtxData, size_t& vtxBufferSizeInBytes, const void* idxData, size_t& idxBufferSizeInBytes)
{
	/* Compute a good alignment */
	size_t total_size_bytes = vtxBufferSizeInBytes + idxBufferSizeInBytes;

	// Staging buffer
	Buffer stagingBuffer;
	stagingBuffer.init(Buffer::Type::STAGING, total_size_bytes, "Vertex/Index Staging Buffer");
	stagingBuffer.create();
	// Copy vertex + index data to staging buffer
	
	void* pData = stagingBuffer.map(context.device, 0, total_size_bytes);
	memcpy(pData, vtxData, vtxBufferSizeInBytes);
	memcpy((unsigned char*)pData + vtxBufferSizeInBytes, idxData, idxBufferSizeInBytes);
	stagingBuffer.unmap(context.device);

	// Create storage buffer containing non-interleaved vertex + index data 
	result.init(Buffer::Type::STORAGE, total_size_bytes, "Vertex/Index SSBO");
	result.create();
	copy_from_buffer(stagingBuffer, result, total_size_bytes);

	return total_size_bytes;
}

VkWriteDescriptorSet BufferWriteDescriptorSet(VkDescriptorSet descriptor_set, uint32_t binding, const VkDescriptorBufferInfo& desc_info, VkDescriptorType desc_type)
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

VkWriteDescriptorSet ImageWriteDescriptorSet(VkDescriptorSet& descriptorSet, uint32_t bindingIndex, const VkDescriptorImageInfo& imageInfo, VkDescriptorType type, uint32_t array_offset, uint32_t descriptor_count)
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

void create_sampler(VkDevice device, VkFilter min, VkFilter mag, VkSamplerAddressMode addressMode, VkSampler& out_Sampler)
{
	VkSamplerCreateInfo sampler_create_info = {};

	sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter = mag;
	sampler_create_info.minFilter = min;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = addressMode;
	sampler_create_info.addressModeV = addressMode;
	sampler_create_info.addressModeW = addressMode;
	sampler_create_info.mipLodBias = 0.0f;
	sampler_create_info.anisotropyEnable = VK_FALSE;
	sampler_create_info.maxAnisotropy = 1.0f;
	sampler_create_info.compareEnable = VK_FALSE;
	sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_create_info.minLod = 0.0f;
	sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
	sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_create_info.unnormalizedCoordinates = VK_FALSE;
	
	VK_CHECK(vkCreateSampler(context.device, &sampler_create_info, nullptr, &out_Sampler));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_sampler(out_Sampler);
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

void set_viewport_scissor(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, bool flip_y)
{
	VkViewport viewport
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)width,
		.height = (float)height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	if (flip_y)
	{
		viewport.y = (float)height;
		viewport.height *= -1;
	}

	VkRect2D scissor = { .offset = {0, 0}, .extent = { width, height} };

	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void set_polygon_mode(VkCommandBuffer cmdBuffer, VkPolygonMode mode)
{
	//fpCmdSetPolygonModeEXT(cmdBuffer, mode);
}

[[nodiscard]] VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descSetLayout, std::span<VkPushConstantRange> push_constant_ranges)
{
	std::array<VkDescriptorSetLayout, 1> layout = { descSetLayout };
	return create_pipeline_layout(device, layout, push_constant_ranges);
}

[[nodiscard]] VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout> desc_set_layouts, std::span<VkPushConstantRange> push_constant_ranges )
{
	VkPipelineLayout out;

	const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = (uint32_t)desc_set_layouts.size(),
		.pSetLayouts = desc_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t)push_constant_ranges.size(),
		.pPushConstantRanges = push_constant_ranges.data()
	};

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &out));

	/* Add to resource manager */
	VkResourceManager::get_instance(context.device)->add_pipeline_layout(out);

	return out;
}