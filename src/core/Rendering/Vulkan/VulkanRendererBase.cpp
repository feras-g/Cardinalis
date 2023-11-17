#include "VulkanRendererBase.h"

void VulkanRendererCommon::init()
{
	create_buffers();
	create_descriptor_sets();
	create_samplers();

	/* Size in pixels of the offscreen buffers */
	render_width = 2048;
	render_height = 2048;

	/* Formats */
	swapchain_color_format = VK_FORMAT_B8G8R8A8_SRGB;
	swapchain_colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchain_depth_format = VK_FORMAT_D32_SFLOAT;
	tex_base_color_format = VK_FORMAT_R8G8B8A8_SRGB;
	tex_metallic_roughness_format = VK_FORMAT_R8G8B8A8_UNORM;
	tex_normal_map_format = VK_FORMAT_R8G8B8A8_UNORM;
	tex_emissive_format = VK_FORMAT_R8G8B8A8_SRGB;
}

void VulkanRendererCommon::create_descriptor_sets()
{
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
	};

	VkDescriptorPool pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);
	m_framedata_desc_set_layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "Framedata UBO");
	m_framedata_desc_set_layout.create("Framedata layout");

	/* Frame data UBO */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_framedata_desc_set[frame_idx].assign_layout(m_framedata_desc_set_layout);
		m_framedata_desc_set[frame_idx].create(pool, "Framedata descriptor set");
		
		VkDescriptorBufferInfo info = { m_ubo_framedata[frame_idx],  0, sizeof(VulkanRendererCommon::FrameData) };
		VkWriteDescriptorSet write = BufferWriteDescriptorSet(m_framedata_desc_set[frame_idx].vk_set, 0, info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
	}
}

void VulkanRendererCommon::create_samplers()
{
	create_sampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	create_sampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatNearest);
	create_sampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	create_sampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}

void VulkanRendererCommon::create_buffers()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_ubo_framedata[frame_idx].init(Buffer::Type::UNIFORM, sizeof(FrameData), "FrameData UBO");
		m_ubo_framedata[frame_idx].create();
	}
}

void VulkanRendererCommon::update_frame_data(const FrameData& data, size_t current_frame_idx)
{
	m_ubo_framedata[current_frame_idx].upload(context.device, (void*)&data, 0, sizeof(data));
}

VulkanRendererCommon::~VulkanRendererCommon()
{
	//vkDeviceWaitIdle(context.device);
	//vkDestroySampler(context.device, s_SamplerRepeatLinear, nullptr);
	//vkDestroySampler(context.device, s_SamplerClampLinear, nullptr);
	//vkDestroySampler(context.device, s_SamplerClampNearest, nullptr);
	//vkDestroySampler(context.device, s_SamplerRepeatNearest, nullptr);
}


