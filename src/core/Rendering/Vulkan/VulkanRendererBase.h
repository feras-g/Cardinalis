#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include <Rendering/Vulkan/VulkanRenderInterface.h>

// Base class to derive to describe a rendering layer
// Example of layers : UI rendering, post-process, different kinds of passes ... 
class VulkanRendererBase
{
public:
	VulkanRendererBase() = default;
	VulkanRendererBase(const VulkanContext& vkContext, bool useDepth);

	virtual ~VulkanRendererBase();
	virtual void PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) = 0; // Write render commands here
	bool RecreateFramebuffersRenderPass();

protected:
	bool bUseDepth;
	virtual bool CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts);
	virtual bool UpdateDescriptorSets(VkDevice device);
	virtual bool CreateRenderPass() = 0;
	virtual bool CreateFramebuffers() = 0;
	bool CreateUniformBuffers(size_t uniformDataSize);
	
	RenderPassInitInfo m_RenderPassInitInfo;
	VkExtent2D m_FramebufferExtent;

protected:
	// Resource descriptors
	VkDescriptorSetLayout	m_DescriptorSetLayout			= VK_NULL_HANDLE;
	VkDescriptorPool		m_DescriptorPool				= VK_NULL_HANDLE;
	VkDescriptorSet			m_DescriptorSets[NUM_FRAMES]	= { VK_NULL_HANDLE };
	
	// Render-Pass / Pipeline state
	VkRenderPass		m_RenderPass				= VK_NULL_HANDLE;
	std::array<VkFramebuffer, NUM_FRAMES> m_Framebuffers	= { VK_NULL_HANDLE };
	VkPipelineLayout	m_PipelineLayout			= VK_NULL_HANDLE;
	VkPipeline			m_GraphicsPipeline			= VK_NULL_HANDLE;
	
	// Uniform buffers
	VkBuffer		m_UniformBuffers[NUM_FRAMES]		= { VK_NULL_HANDLE };
	VkDeviceMemory	m_UniformBuffersMemory[NUM_FRAMES]	= { VK_NULL_HANDLE };

	// Samplers
	static void CreateSamplers();

	static VkSampler s_SamplerRepeatLinear;
	static VkSampler s_SamplerClampLinear;
	static VkSampler s_SamplerClampNearest;
};

#endif // !RENDERER_BASE_H
