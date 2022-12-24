#include "VulkanRendererBase.h"

VkSampler VulkanRendererBase::s_SamplerRepeatLinear;
VkSampler VulkanRendererBase::s_SamplerClampLinear;
VkSampler VulkanRendererBase::s_SamplerClampNearest;

VulkanRendererBase::VulkanRendererBase(const VulkanContext& vkContext, bool useDepth)
	: bUseDepth(useDepth)
{
	CreateSamplers();
}

VulkanRendererBase::~VulkanRendererBase()
{
	vkDeviceWaitIdle(context.device);
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		vkDestroyBuffer(context.device, m_UniformBuffers[i], nullptr);
	}

	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		vkFreeMemory(context.device, m_UniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorSetLayout(context.device, m_DescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(context.device, m_DescriptorPool, nullptr);
	vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(context.device, m_GraphicsPipeline, nullptr);
}

bool VulkanRendererBase::RecreateFramebuffersRenderPass()
{
	for (int i=0; i < m_Framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(context.device, m_Framebuffers[i], nullptr);
	}
	
	vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	CreateColorDepthRenderPass(m_RenderPassInitInfo, bUseDepth, &m_RenderPass);
	CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth);
	
	return true;
}

bool VulkanRendererBase::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	// By default, this creates a descriptor set for swapchain buffer containing 4 descriptors 
	// 1 Uniform buffer, 2 for Storage buffers, 1 combined image sampler

	// Specify layouts
	constexpr uint32_t bindingCount = 4;
	VkDescriptorSetLayoutBinding bindings[bindingCount] =
	{
		{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = 0,
		.bindingCount = bindingCount,
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	std::array<VkDescriptorSetLayout, NUM_FRAMES> layouts = { m_DescriptorSetLayout, m_DescriptorSetLayout };	// 1 descriptor set per frame

	// Allocate memory
	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_DescriptorPool,
		.descriptorSetCount = NUM_FRAMES,
		.pSetLayouts = layouts.data(),
	};

	return (vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets) == VK_SUCCESS);
}

bool VulkanRendererBase::UpdateDescriptorSets(VkDevice device)
{
	return false;
}

bool VulkanRendererBase::CreateUniformBuffers(size_t uniformDataSize)
{
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		if (!CreateUniformBuffer(uniformDataSize, m_UniformBuffers[i], m_UniformBuffersMemory[i]))
		{
			LOG_ERROR("Failed to create uniform buffers.");
			return false;
		}
	}

	return true;
}

void VulkanRendererBase::CreateSamplers()
{
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}
