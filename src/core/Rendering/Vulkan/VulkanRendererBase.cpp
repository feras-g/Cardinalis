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
		DestroyBuffer(m_UniformBuffers[i]);
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
	CreateColorDepthRenderPass(m_RenderPassInitInfo, &m_RenderPass);
	CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), bUseDepth);
	
	return true;
}

bool VulkanRendererBase::CreateDescriptorSets(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings,
	VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = 0,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings	  = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	// 1 descriptor set per frame
	std::array<VkDescriptorSetLayout, NUM_FRAMES> layouts;
	layouts.fill(m_DescriptorSetLayout);

	// Allocate memory
	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_DescriptorPool,
		.descriptorSetCount = NUM_FRAMES,
		.pSetLayouts = layouts.data(),
	};

	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets));

	return true;
}

bool VulkanRendererBase::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	// Specify layouts
	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
	};

	return CreateDescriptorSets(device, bindings, out_DescriptorSets, out_DescLayouts);
}

bool VulkanRendererBase::UpdateDescriptorSets(VkDevice device)
{
	return false;
}

bool VulkanRendererBase::CreateUniformBuffers(size_t size)
{
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		CreateUniformBuffer(m_UniformBuffers[i], size);
	}

	return true;
}

void VulkanRendererBase::CreateSamplers()
{
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}
