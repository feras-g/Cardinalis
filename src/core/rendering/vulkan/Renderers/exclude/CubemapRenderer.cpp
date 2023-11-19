#include "CubemapRenderer.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/DescriptorSet.h"
#include "DeferredRenderer.h"

#include <glm/gtx/string_cast.hpp>

/*
	Basic steps for skybox rendering from equirectangular map :
	- Load an equirectangular map
	- Create a color attachment containing 6 layers
	- Render the equirect map to each layer, with a different view corresponding to each face
	- Render cube mesh and sample from cube texture
*/

/* 2D texture with 6 layers interpreted as a cubemap */
static Texture2D cubemap_render_attachment;
VkFormat cubemap_render_attachment_format = VK_FORMAT_R32G32B32A32_SFLOAT;

/* Generate a cubemap using VK_KHR_multiview */
static const uint32_t view_mask = 0b00111111;
static VulkanRenderPassDynamic pass_render_cubemap;

/* Generate the irradiance map from the generated cubemap*/
static Texture2D irradiance_map_attachment;
VkFormat irradiance_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;
static VulkanRenderPassDynamic pass_render_irradiance_map;
uint32_t irradiance_map_dim = 32;


DescriptorSet irradiance_desc_set;
VkPipelineLayout render_irradiance_ppl_layout;
VkPipeline render_irradiance_gfx_ppl;

/* Render skybox sampling cubemap */
static VulkanRenderPassDynamic pass_render_skybox[NUM_FRAMES];


/* Params */
uint32_t layer_width  = 0;
uint32_t layer_height = 0;

Buffer ubo_cube_matrices;
struct CubeMatrices
{
	/* View matrices for each cube face */
	glm::mat4 view_matrices[6]
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, 1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f))
	};
	glm::mat4 cube_model;
	glm::mat4 projection { glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) };
} cube_matrices;

VkDescriptorPool pool;

/* Descriptor */

DescriptorSet		cubemap_desc_set;
VkPipelineLayout    pipeline_layout;
VkPipeline			gfx_ppl;

/* Descriptor : Render skybox */
DescriptorSet render_skybox_desc_set;
VkPipeline			render_skybox_gfx_ppl;
VkPipelineLayout    render_skybox_ppl_layout;

/* Drawable */
//Drawable* d_skybox;
//Drawable* d_placeholder_cube;

/* Cubemap rendering */
void CubemapRenderer::init()
{
	init_resources("../../../data/textures/env/san_giuseppe_bridge_4k.hdr");
	init_descriptors();

	pass_render_cubemap.add_color_attachment(cubemap_render_attachment.view);

	/* Cubemap generation shader */
	VertexFragmentShader shader_gen_cubemap;
	shader_gen_cubemap.create("GenCubemap.vert.spv", "GenCubemap.frag.spv");

	/* Graphics pipeline */
	Pipeline::Flags flags = Pipeline::Flags::NONE;
	VkFormat color_formats[1] = { cubemap_render_attachment_format };
	//Pipeline::create_graphics_pipeline_dynamic(shader_gen_cubemap, color_formats, {}, flags, pipeline_layout, &gfx_ppl, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, view_mask);

	TransformData transform;
	transform.model = glm::identity<glm::mat4>();
	//d_skybox = RenderObjectManager::get_drawable("skybox");
	//d_placeholder_cube = RenderObjectManager::get_drawable("placeholder_cube");

	VkCommandBuffer tmp = begin_temp_cmd_buffer();

	VkRect2D area{ .offset = { } , .extent {.width = layer_width, .height = layer_height } };
	/* Render equirectangular map to a cubemap */
	render_cubemap(tmp, area);

	/* Compute the irradiance map for this cubemap */
	init_irradiance_map_rendering();
	render_irradiance_map(tmp);

	end_temp_cmd_buffer(tmp);

	init_skybox_rendering();
	
	LOG_INFO("Matrix :{}", glm::to_string(cube_matrices.projection));
}

/* Irradiance map rendering */
void CubemapRenderer::init_irradiance_map_rendering()
{
	VertexFragmentShader shader_gen_irradiance_map;
	shader_gen_irradiance_map.create("GenCubemap.vert.spv", "ConvoluteCubemap.frag.spv");

	/* Irrandiance map attachment */
	irradiance_map_attachment.init(irradiance_map_format, irradiance_map_dim, irradiance_map_dim, 6, false, "Irradiance Map");
	irradiance_map_attachment.create_vk_image(context.device, true, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	irradiance_map_attachment.view = Texture2D::create_texture_2d_array_view(irradiance_map_attachment, irradiance_map_attachment.info.imageFormat);

	pass_render_irradiance_map.add_color_attachment(irradiance_map_attachment.view);

	/* sampler cube view */
	tex_irradiance_map_view = Texture2D::create_texture_cube_view(
		irradiance_map_attachment, irradiance_map_attachment.info.imageFormat);

	VkSampler sampler = VulkanRendererCommon::get_instance().s_SamplerClampLinear;
	irradiance_desc_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sampler, "Cubemap");
	irradiance_desc_set.create(pool, "Gen Irradiance");

	//irradiance_desc_set.write_descriptor_combined_image_sampler(0, tex_irradiance_map_view, sampler);
	//irradiance_desc_set.update();

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = tex_cubemap_view;
	image_info.sampler = sampler;

	VkWriteDescriptorSet write = ImageWriteDescriptorSet(irradiance_desc_set.vk_set, 0, image_info);
	vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);

	/* Pipeline layout */
	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	//desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);
	desc_set_layouts.push_back(cubemap_desc_set.layout);
	desc_set_layouts.push_back(irradiance_desc_set.layout);
	render_irradiance_ppl_layout = create_pipeline_layout(context.device, desc_set_layouts);

	/* Graphics pipeline */
	Pipeline::Flags flags = Pipeline::Flags::NONE;
	VkFormat color_formats[1] = { irradiance_map_format };
	//Pipeline::create_graphics_pipeline_dynamic(shader_gen_irradiance_map, color_formats, {}, flags, render_irradiance_ppl_layout, &render_irradiance_gfx_ppl, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, view_mask);
}

void CubemapRenderer::render_irradiance_map(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Irradiance map Generation Render Pass");

	set_viewport_scissor(cmd_buffer, irradiance_map_dim, irradiance_map_dim, true);

	//vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_irradiance_ppl_layout, 0, 1, &d_placeholder_cube->get_mesh().descriptor_set, 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_irradiance_ppl_layout, 1, 1, &cubemap_desc_set.vk_set, 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_irradiance_ppl_layout, 2, 1, &irradiance_desc_set.vk_set, 0, nullptr);

	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_irradiance_gfx_ppl);
	
	cubemap_render_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	irradiance_map_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	pass_render_irradiance_map.begin(cmd_buffer, { irradiance_map_dim, irradiance_map_dim }, view_mask);

	//d_placeholder_cube->draw(cmd_buffer);

	pass_render_irradiance_map.end(cmd_buffer);

	irradiance_map_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}

/* Skybox rendering */
void CubemapRenderer::init_skybox_rendering()
{
	VertexFragmentShader shader_render_skybox;
	shader_render_skybox.create("Skybox.vert.spv", "Skybox.frag.spv");

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		pass_render_skybox[i].add_color_attachment(VulkanRendererCommon::get_instance().m_deferred_lighting_attachment[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
		pass_render_skybox[i].add_depth_attachment(VulkanRendererCommon::get_instance().m_gbuffer_depth[i].view, VK_ATTACHMENT_LOAD_OP_LOAD);
	}
	VkSampler sampler = VulkanRendererCommon::get_instance().s_SamplerClampLinear;

	///* Descriptor */
	render_skybox_desc_set.layout.add_combined_image_sampler_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sampler, "SamplerCube");
	render_skybox_desc_set.create(pool, "Render Skybox");
	
	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = tex_cubemap_view;// tex_irradiance_map_view;
	image_info.sampler = sampler;

	VkWriteDescriptorSet write = ImageWriteDescriptorSet(render_skybox_desc_set.vk_set, 0, image_info);
	vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	//desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);
	desc_set_layouts.push_back(VulkanRendererCommon::get_instance().m_framedata_desc_set_layout);
	desc_set_layouts.push_back(render_skybox_desc_set.layout);
	render_skybox_ppl_layout = create_pipeline_layout(context.device, desc_set_layouts);

	/* Graphics pipeline */
	Pipeline::Flags flags = Pipeline::Flags::ENABLE_DEPTH_STATE;
	VkFormat color_formats[1] = { VulkanRendererCommon::get_instance().tex_deferred_lighting_format };
	//Pipeline::create_graphics_pipeline_dynamic(shader_render_skybox, color_formats, VulkanRendererBase::tex_gbuffer_depth_format, flags, render_skybox_ppl_layout, &render_skybox_gfx_ppl, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, {});
}

void CubemapRenderer::init_resources(const char* filename)
{
	/* Equirectangular map */
	//equirect_map_id = RenderObjectManager::add_texture(filename, "Cubemap_Newport_Loft_HDR", VK_FORMAT_R32G32B32A32_SFLOAT, false);
	//
	//uint32_t dim = std::max(RenderObjectManager::textures[equirect_map_id].info.width, RenderObjectManager::textures[equirect_map_id].info.height);

	//layer_height = layer_width = dim;

	/* Cubemap render attachment */
	cubemap_render_attachment.init(cubemap_render_attachment_format, layer_width, layer_height, 6, false, "Cubemap Array Texture");
	cubemap_render_attachment.create_vk_image(context.device, true, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	cubemap_render_attachment.view = Texture2D::create_texture_2d_array_view(cubemap_render_attachment, cubemap_render_attachment.info.imageFormat);

	/* Uniform buffer */
	cube_matrices.cube_model = glm::identity<glm::mat4>();

	ubo_cube_matrices.init(Buffer::Type::UNIFORM, sizeof(cube_matrices), "Cubemap Matrices UBO");
	ubo_cube_matrices.upload(context.device, &cube_matrices, 0, sizeof(cube_matrices));
}

void CubemapRenderer::init_descriptors()
{
	pool = create_descriptor_pool(0, 0, 1, 2, 0, 3);
	
	VkSampler& sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatNearest;
	
	cubemap_desc_set.layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT, "View matrices UBO");
	cubemap_desc_set.layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sampler, "HDR map texture");
	cubemap_desc_set.layout.create("GenCubemap");

	cubemap_desc_set.create(pool, "GenCubemap");

	//cubemap_desc_set.write_descriptor_uniform_buffer(0, uniform_buffer.buffer, 0, sizeof(ubo));
	//cubemap_desc_set.write_descriptor_combined_image_sampler(1, RenderObjectManager::textures[equirect_map_id].view, sampler);
	//cubemap_desc_set.update();

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.offset = 0;
	buffer_info.range = sizeof(cube_matrices);
	buffer_info.buffer = ubo_cube_matrices;

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//image_info.imageView = RenderObjectManager::textures[equirect_map_id].view;
	image_info.sampler = sampler;

	std::vector<VkWriteDescriptorSet> write_descriptor_set = {};
	write_descriptor_set.push_back(BufferWriteDescriptorSet(cubemap_desc_set.vk_set, 0, buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
	write_descriptor_set.push_back(ImageWriteDescriptorSet(cubemap_desc_set.vk_set, 1, image_info));

	vkUpdateDescriptorSets(context.device, 2, write_descriptor_set.data(), 0, nullptr);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	//desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);								
	desc_set_layouts.push_back(cubemap_desc_set.layout);	
	pipeline_layout = create_pipeline_layout(context.device, desc_set_layouts);
}

void init_gfx_pipeline()
{

}
void render(VkCommandBuffer cmd_buffer, VkRect2D area)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Skybox Draw Pass");
}

/* Generate cubemap using VK_KHR_multiview to project the equirectangular map on each cube face */
void CubemapRenderer::render_cubemap(VkCommandBuffer cmd_buffer, VkRect2D area)
{	
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Cubemap Generation Render Pass");
	
	set_viewport_scissor(cmd_buffer, layer_width, layer_height, true);
	VkDescriptorSet desc_sets[2] =
	{
		//d_placeholder_cube->get_mesh().descriptor_set,
		cubemap_desc_set.vk_set
	};

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2, desc_sets, 0, nullptr);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx_ppl);

	cubemap_render_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	pass_render_cubemap.begin(cmd_buffer, {area.extent.width, area.extent.height }, view_mask);

	//d_placeholder_cube->draw(cmd_buffer);

	pass_render_cubemap.end(cmd_buffer);

	cubemap_render_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	
	/* Sampler cube */
	tex_cubemap_view = Texture2D::create_texture_cube_view(
		cubemap_render_attachment, cubemap_render_attachment.info.imageFormat);
}


void CubemapRenderer::render_skybox(size_t currentImageIdx, VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Skybox Render Pass");

	set_viewport_scissor(cmd_buffer, VulkanRendererCommon::get_instance().render_width, VulkanRendererCommon::get_instance().render_height, true);

	const VkDescriptorSet descriptor_sets[3] 
	{
		//d_skybox->get_mesh().descriptor_set,
		VulkanRendererCommon::get_instance().m_framedata_desc_set[currentImageIdx].vk_set,
		render_skybox_desc_set.vk_set
	};

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_skybox_ppl_layout, 0, 3, descriptor_sets, 0, nullptr);

	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_skybox_gfx_ppl);

	pass_render_skybox[currentImageIdx].begin(cmd_buffer, { VulkanRendererCommon::get_instance().render_width, VulkanRendererCommon::get_instance().render_height });

	//d_skybox->draw(cmd_buffer);

	pass_render_skybox[currentImageIdx].end(cmd_buffer);
}