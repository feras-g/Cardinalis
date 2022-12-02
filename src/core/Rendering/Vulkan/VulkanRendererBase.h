#ifndef RENDERER_BASE_H
#define RENDERER_BASE_H

#include <Rendering/Vulkan/VulkanRenderInterface.h>

// Base class to derive to describe a rendering layer
// Example of layers : UI rendering, post-process, different kinds of passes ... 
class VulkanRendererBase
{
public:
	VulkanRendererBase(const VulkanContext& vkContext);

	virtual ~VulkanRendererBase();

	virtual void PopulateCommandBuffer(VkCommandBuffer cmdBuffer) const = 0; // Write render commands here


protected:
	bool CreateColorDepthFramebuffers(VkRenderPass renderPass, VulkanSwapchain& swapchain);
	bool CreateUniformBuffers(size_t uniformDataSize);

	VkExtent2D m_FramebufferExtent;

protected:
	// Resource descriptors
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet m_DescriptorSets[NUM_FRAMES] = { VK_NULL_HANDLE };
	
	// Render-Pass / Pipeline state
	VulkanTexture m_DepthTexture;
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	
	// UBO
	VkBuffer m_UniformBuffers[NUM_FRAMES] = { VK_NULL_HANDLE };
	VkDeviceMemory m_UniformBuffersMemory[NUM_FRAMES] = { VK_NULL_HANDLE };
};

#endif // !RENDERER_BASE_H
