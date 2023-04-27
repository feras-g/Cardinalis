#include "VulkanRendererBase.h"

VkSampler VulkanRendererBase::s_SamplerRepeatLinear;
VkSampler VulkanRendererBase::s_SamplerClampLinear;
VkSampler VulkanRendererBase::s_SamplerClampNearest;
VkSampler VulkanRendererBase::s_SamplerRepeatNearest;
Buffer VulkanRendererBase::m_ubo_common_framedata[NUM_FRAMES];
	
void VulkanRendererBase::create_samplers()
{
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatNearest);
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	CreateTextureSampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}

void VulkanRendererBase::create_buffers()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		create_uniform_buffer(m_ubo_common_framedata[frame_idx], sizeof(PerFrameData));
	}
}

void VulkanRendererBase::update_frame_data(const PerFrameData& data, size_t current_frame_idx)
{
	upload_buffer_data(m_ubo_common_framedata[current_frame_idx], (void*)&data, sizeof(data), 0);
}

void VulkanRendererBase::destroy()
{
	vkDeviceWaitIdle(context.device);
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		destroy_buffer(m_ubo_common_framedata[i]);
	}
	vkDestroySampler(context.device, s_SamplerRepeatLinear, nullptr);
	vkDestroySampler(context.device, s_SamplerClampLinear, nullptr);
	vkDestroySampler(context.device, s_SamplerClampNearest, nullptr);
	vkDestroySampler(context.device, s_SamplerRepeatNearest, nullptr);
}
