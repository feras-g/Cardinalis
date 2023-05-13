#include "CubemapRenderer.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"

/*
	Basic steps:
	- Load an equirectangular map
	- Create a color attachment containing 6 layers
	- Render the equirect map to each layer, with a different view corresponding to each face
	- Render cube mesh and sample from cube texture
*/

/* 2D texture with 6 layers interpreted as a cubemap */
static Texture2D cubemap_render_attachment;

/* Render a cubemap using VK_KHR_multiview */
static vk::DynamicRenderPass pass_render_cubemap;

static glm::mat4 proj_matrix = glm::perspective(90.0f, 1.0f, 0.1f, 1.0f);

/* View matrices for each cube face */
static glm::mat4 view_matrices[6]
{
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y  
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))	 // -Z
};

void CubemapRenderer::init()
{
	load_resources("../../../data/textures/env/kloofendal_48d_partly_cloudy_puresky_1k.hdr");

	pass_render_cubemap.add_color_attachment(cubemap_render_attachment.view);
}

void CubemapRenderer::load_resources(const char* filename)
{
	/* Params */
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	uint32_t width  = 512;
	uint32_t height = 512;

	/* Equirectangular map */
	equirect_map_id = RenderObjectManager::add_texture(filename, "Cubemap_Newport_Loft_HDR", VK_FORMAT_R32G32B32A32_SFLOAT);

	/* Cubemap attachment */
	cubemap_render_attachment.init(format, width, height, 6, true);
	cubemap_render_attachment.create_vk_image_cube(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	cubemap_render_attachment.create_view(context.device, ImageViewCubeTexture);
}

void render(VkCommandBuffer cmd_buffer, VkRect2D area)
{

}

/* Using VK_KHR_multiview to project the equirectangular map on each cube face */
void render_cubemap(VkCommandBuffer cmd_buffer, VkRect2D area)
{
	pass_render_cubemap.begin(cmd_buffer, area);

	pass_render_cubemap.end(cmd_buffer);
}
