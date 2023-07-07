#include "PostProcessingRenderer.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "Rendering/Vulkan/VulkanTexture.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"

void PostProcessRenderer::init()
{
	
}

void PostProcessRenderer::render(PostFX fx, VkCommandBuffer_T* cmd_buff, const Texture2D& input_image)
{
	if (fx == PostFX::eDownsample)
	{
		m_postfx_downsample.render(cmd_buff);
	}
}

static Texture2D create_test_texture()
{
	Texture2D texture;
	constexpr int dim = 65;
	texture.init(VK_FORMAT_R8G8B8A8_UNORM, dim, dim, 1, false);

	constexpr int bpp = 4;
	static std::array<glm::tvec4<uint8_t>, dim*dim> data;
	data.fill({0,0,0,0});
	data[(dim / 2) * dim + (dim / 2)] = { 255,255,255,255 }; // Center pixel

	constexpr int total_size_bytes = dim * dim * bpp;
	static std::array<uint8_t, total_size_bytes> flat_data;

	for (size_t i = 0, j = 0; i < dim * dim; i++, j+=4)
	{
		flat_data[j]    = data[i].x;
		flat_data[j+1]  = data[i].y;
		flat_data[j+2]  = data[i].z;
		flat_data[j+3]  = data[i].w;
	}
	
	texture.create_from_data(flat_data.data(), "Post Process Test texture", VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
	texture.create_view(context.device, ImageViewTexture2D);

	return texture;
}

void PostFX_Downsample::init()
{
	Texture2D& input_image = VulkanRendererBase::m_gbuffer_albedo[0];
	input_image_handle = &input_image;// &input_image;
	width  = input_image.info.width;
	height = input_image.info.height;

	m_downsample_cs.create("Downsample.comp.spv");
	output_image.init(input_image.info.imageFormat, width / 2, height / 2, 1, false);
	output_image.create(context.device, VK_IMAGE_USAGE_STORAGE_BIT, "PostFX_Downsample Ouput Storage Image");
	output_image.create_view(context.device, ImageViewTexture2D);

	output_image.transition_layout_immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	
	/* Descriptor set */
	DescriptorSetLayout desc_set_layout;
	desc_set_layout.add_storage_image_binding(0, "Read-only Input image");
	desc_set_layout.add_storage_image_binding(1, "Write Ouput image");
	desc_set_layout.create("PostFX_Downsample Descriptor Set Layout");
	
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 }
	};
	VkDescriptorPool pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

	desc_set.assign_layout(desc_set_layout);
	desc_set.create(pool, "PostFX_Downsample Descriptor Set Layout");

	VkDescriptorImageInfo input_image_desc_info = { VK_NULL_HANDLE, input_image.view,  VK_IMAGE_LAYOUT_GENERAL  };
	VkDescriptorImageInfo ouput_image_desc_info = { VK_NULL_HANDLE, output_image.view, VK_IMAGE_LAYOUT_GENERAL  };

	std::vector<VkWriteDescriptorSet> write_descriptor_set 
	{
		ImageWriteDescriptorSet(desc_set.set, 0, &input_image_desc_info, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE),
		ImageWriteDescriptorSet(desc_set.set, 1, &ouput_image_desc_info, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
	};
	vkUpdateDescriptorSets(context.device, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo ppl_layout_info = {};
	ppl_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ppl_layout_info.setLayoutCount = 1;
	ppl_layout_info.pSetLayouts = &desc_set_layout.layout;
	
	vkCreatePipelineLayout(context.device, &ppl_layout_info, nullptr, &ppl_layout);

	/* Compute pipeline */
	VkComputePipelineCreateInfo compute_ppl_info = {};
	compute_ppl_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_ppl_info.stage = m_downsample_cs.stages[0];
	compute_ppl_info.layout = ppl_layout;

	vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &compute_ppl_info, nullptr, &compute_ppl);
}

void PostFX_Downsample::render(VkCommandBuffer_T* cmd_buff)
{
	static int workgroup_dim = 8;
	static int num_threads_X = width / 2;
	static int num_threads_Y = height / 2;

	static int dispatch_size_x = num_threads_X / workgroup_dim;
	static int dispatch_size_y = num_threads_Y / workgroup_dim;

	input_image_handle->transition_layout(cmd_buff, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, ppl_layout, 0, 1, &desc_set.set, 0, nullptr);
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, compute_ppl);
	vkCmdDispatch(cmd_buff, dispatch_size_x, dispatch_size_y, 1);

	input_image_handle->transition_layout(cmd_buff, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
