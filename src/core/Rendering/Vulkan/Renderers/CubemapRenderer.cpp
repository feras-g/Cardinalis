#include "CubemapRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/DescriptorSet.h"

/*
	Basic steps:
	- Load an equirectangular map
	- Create a color attachment containing 6 layers
	- Render the equirect map to each layer, with a different view corresponding to each face
	- Render cube mesh and sample from cube texture
*/

/* 2D texture with 6 layers interpreted as a cubemap */
static Texture2D cubemap_render_attachment;
VkFormat cubemap_render_attachment_format = VK_FORMAT_R32G32B32A32_SFLOAT;

/* Render a cubemap using VK_KHR_multiview */
static const uint32_t view_mask = 0b00111111;
static vk::DynamicRenderPass pass_render_cubemap;

static glm::mat4 proj_matrix = glm::perspective(90.0f, 1.0f, 0.1f, 1.0f);

/* Params */
uint32_t layer_width  = 512;
uint32_t layer_height = 512;

Buffer uniform_buffer;
struct UBO
{
	/* View matrices for each cube face */
	glm::mat4 view_matrices[6]
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y  
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))	 // -Z
	};
	glm::mat4 cube_model;
	glm::mat4 projection;
} ubo;

/* Descriptor */

DescriptorSetLayout cubemap_desc_layout;
DescriptorSet		cubemap_desc_set;
VkPipelineLayout    ppl_layout;
VkPipeline			gfx_ppl;

/* Drawable */
Drawable* unit_cube;

void CubemapRenderer::init()
{
	//init_resources("../../../data/textures/env/kloofendal_48d_partly_cloudy_puresky_1k.hdr");
	init_resources("../../../data/textures/env/newport_loft.hdr");
	init_descriptors();

	pass_render_cubemap.add_color_attachment(cubemap_render_attachment.view);

	/* Cubemap generation shader */
	VulkanShader shader("GenCubemap.vert.spv", "GenCubemap.frag.spv");

	/* Graphics pipeline */
	GfxPipeline::Flags flags = GfxPipeline::Flags::NONE;
	VkFormat color_formats[1] = { cubemap_render_attachment_format };
	GfxPipeline::CreateDynamic(shader, color_formats, {}, flags, ppl_layout, &gfx_ppl, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, {}, view_mask);

	TransformData transform;
	transform.model = glm::identity<glm::mat4>();
	unit_cube = RenderObjectManager::get_drawable("unit_cube");

	VkRect2D area  { .offset = { } , .extent {.width = layer_width, .height = layer_height } };
	render_cubemap(area);
}

void CubemapRenderer::init_resources(const char* filename)
{
	/* Equirectangular map */
	equirect_map_id = RenderObjectManager::add_texture(filename, "Cubemap_Newport_Loft_HDR", VK_FORMAT_R32G32B32A32_SFLOAT);
	
	/* Cubemap attachment */
	cubemap_render_attachment.init(cubemap_render_attachment_format, layer_width, layer_height, 6, true);
	cubemap_render_attachment.create_vk_image(context.device, false, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	cubemap_render_attachment.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1  });

	/* Uniform buffer */
	create_uniform_buffer(uniform_buffer, sizeof(ubo));

	ubo.projection = proj_matrix;
	ubo.cube_model = glm::identity<glm::mat4>();

	upload_buffer_data(uniform_buffer, &ubo, sizeof(ubo), 0);
}

void CubemapRenderer::init_descriptors()
{
	VkDescriptorPool pool = create_descriptor_pool(0, 1, 1, 0, NUM_FRAMES);
	
	const VkSampler& sampler = VulkanRendererBase::s_SamplerRepeatNearest;
	
	cubemap_desc_layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT, "View matrices UBO");
	cubemap_desc_layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, sampler, "HDR map texture");
	cubemap_desc_layout.create("GenCubemap");

	cubemap_desc_set.assign_layout(cubemap_desc_layout);
	cubemap_desc_set.create(pool, "GenCubemap");

	//cubemap_desc_set.write_descriptor_uniform_buffer(0, uniform_buffer.buffer, 0, sizeof(ubo));
	//cubemap_desc_set.write_descriptor_combined_image_sampler(1, RenderObjectManager::textures[equirect_map_id].view, sampler);
	//cubemap_desc_set.update();

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.offset = 0;
	buffer_info.range = sizeof(ubo);
	buffer_info.buffer = uniform_buffer.buffer;

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = RenderObjectManager::textures[equirect_map_id].view;
	image_info.sampler = sampler;

	std::vector<VkWriteDescriptorSet> write_descriptor_set = {};
	write_descriptor_set.push_back(BufferWriteDescriptorSet(cubemap_desc_set.set, 0, &buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(cubemap_desc_set.set, 1, &image_info));

	vkUpdateDescriptorSets(context.device, 2, write_descriptor_set.data(), 0, nullptr);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);								
	desc_set_layouts.push_back(cubemap_desc_layout.layout);	
	ppl_layout = create_pipeline_layout(context.device, desc_set_layouts);
}

void init_gfx_pipeline()
{

}
void render(VkCommandBuffer cmd_buffer, VkRect2D area)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Skybox Draw Pass");
}

/* Generate cubemap using VK_KHR_multiview to project the equirectangular map on each cube face */
void CubemapRenderer::render_cubemap(VkRect2D area)
{	
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	{
		VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Cubemap Generation Render Pass");
		
		set_viewport_scissor(cmd_buffer, layer_width, layer_height);
		VkDescriptorSet desc_sets[2] =
		{
			unit_cube->mesh_handle->descriptor_set,
			cubemap_desc_set.set
		};

		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ppl_layout, 0, 2, desc_sets, 0, nullptr);
		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx_ppl);

		/* The ith least significant view correpondons to the ith layer */
		pass_render_cubemap.begin(cmd_buffer, area, view_mask);

		unit_cube->draw(cmd_buffer);

		pass_render_cubemap.end(cmd_buffer);
	}

	end_temp_cmd_buffer(cmd_buffer);
}
