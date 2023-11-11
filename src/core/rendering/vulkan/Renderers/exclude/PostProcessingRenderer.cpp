#include "PostProcessingRenderer.h"
#include "core/rendering/vulkan/VulkanShader.h"
#include "core/rendering/vulkan/VulkanTexture.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"

/* 
	Generic descriptor set for post-processing.
*/
static void create_descriptor_set_compute_shader(DescriptorSet& descriptor_set)
{
	/* Create default descriptor set */
	descriptor_set.layout.add_storage_image_binding(0, "Read-Only Input image");
	descriptor_set.layout.add_storage_image_binding(1, "Write Ouput image");
	descriptor_set.layout.create("Post Process Renderer");
	
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 }
	};
	VkDescriptorPool pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);

	descriptor_set.create(pool, "PostFX_Downsample Descriptor Set Layout");
}

void PostProcessRenderer::init()
{
	create_descriptor_set_compute_shader(descriptor_set);
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
	texture.init(VK_FORMAT_R8G8B8A8_UNORM, dim, dim, 1, false, "Test texture");

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
	
	texture.create_from_data(flat_data.data(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
	texture.create_view(context.device, ImageViewTexture2D);

	return texture;
}

/*
	Create the pipeline objects for the downsample.
*/
void PostFX_Downsample::init()
{
	/* Descriptor set */
	create_descriptor_set_compute_shader(descriptor_set);

	/* Pipeline layout */
	pipeline_layout = create_pipeline_layout(context.device, descriptor_set.layout, {});

	/* Compute pipeline */
	//pipeline = Pipeline::create_compute_pipeline(shader, pipeline_layout);
}

/*
	Link the descriptors to actual data.
*/
void PostFX_Downsample::update_data(PostFX_Downsample::Data data)
{
	//m_data = data;

	//VkDescriptorImageInfo input_image_desc_info = { VK_NULL_HANDLE, m_data.input_image->view,  VK_IMAGE_LAYOUT_GENERAL };
	//VkDescriptorImageInfo ouput_image_desc_info = { VK_NULL_HANDLE, m_data.output_image->view, VK_IMAGE_LAYOUT_GENERAL };

	//std::vector<VkWriteDescriptorSet> write_descriptor_set
	//{
	//	ImageWriteDescriptorSet(descriptor_set, 0, input_image_desc_info, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE),
	//	ImageWriteDescriptorSet(descriptor_set, 1, ouput_image_desc_info, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
	//};
	//vkUpdateDescriptorSets(context.device, (uint32_t)write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);
}

/*
	Perform the compute pass.
*/
void PostFX_Downsample::render(VkCommandBuffer_T* cmd_buff)
{
	static int workgroup_dim = 8;
	int num_threads_X = m_data.input_image->info.width  / 2;
	int num_threads_Y = m_data.input_image->info.height / 2;

	static int dispatch_size_x = num_threads_X / workgroup_dim;
	static int dispatch_size_y = num_threads_Y / workgroup_dim;

	vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, descriptor_set, 0, nullptr);
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdDispatch(cmd_buff, dispatch_size_x, dispatch_size_y, 1);
}

