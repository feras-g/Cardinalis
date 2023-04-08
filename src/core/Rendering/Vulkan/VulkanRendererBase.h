#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include <Rendering/Vulkan/VulkanRenderInterface.h>
#include <Rendering/Vulkan/RenderPass.h>



class VulkanRendererBase
{
public:
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 mvp;
	} frame_data;

	static void create_samplers();
	static void create_common_buffers();
	static void update_common_framedata(const UniformData& data);

	static VkSampler s_SamplerRepeatLinear;
	static VkSampler s_SamplerClampLinear;
	static VkSampler s_SamplerClampNearest;
	static Buffer m_ubo_common_framedata;
	

	/* WIP */
	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
	VkPipeline m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorSet m_descriptor_set;

	// void init();
	// void render();
	// void draw_scene();
	// void create_buffers()
	// void update_buffers(void* uniform_data, size_t data_size);
	// void update_descriptor_set(VkDevice device);
	// void create_attachments();
};

#endif // !RENDERER_BASE_H
