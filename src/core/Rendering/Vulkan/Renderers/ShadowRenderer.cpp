#include "ShadowRenderer.h"
#include "Rendering/LightManager.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Camera.h"

static constexpr VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;

void ShadowRenderer::init(unsigned int width, unsigned int height, const LightManager& lightmanager)
{
	h_light_manager = &lightmanager;

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	m_shadow_map_size = { width, height };

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, width, height,1, false);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_shadow_maps[frame_idx].create_view(context.device, { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT });
		m_shadow_maps[frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* Create render pass */
		m_shadow_pass[frame_idx].add_depth_attachment(m_shadow_maps[frame_idx].view);
	}

	end_temp_cmd_buffer(cmd_buffer);

	/* Shadow pass set */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Lighting data */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Depth G-Buffer */
	bindings.push_back({ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }); /* Frame data */

	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(m_descriptor_set_layout);								/* Shadow pass descriptor set */
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);		/* Mesh geometry descriptor set*/
	desc_set_layouts.push_back(RenderObjectManager::drawable_descriptor_set_layout);	/* Drawable data descriptor set */
	desc_set_layouts.push_back(VulkanRendererBase::m_framedata_desc_set_layout.layout);		/* Frame data */

	m_descriptor_pool       = create_descriptor_pool(0, 2, 1, 0);
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_descriptor_set[frame_idx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
	}

	update_desc_sets();

	size_t vtx_pushconstantsize = sizeof(glm::mat4);
	m_gfx_pipeline_layout   = create_pipeline_layout(context.device, desc_set_layouts, (uint32_t)vtx_pushconstantsize, 0);

	/* Create graphics pipeline */
	m_shadow_shader.load_from_file("GenShadowMap.vert.spv", "GenShadowMap.frag.spv");
	GfxPipeline::Flags flags = GfxPipeline::Flags::ENABLE_DEPTH_STATE;
	GfxPipeline::CreateDynamic(m_shadow_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void ShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		h_gbuffers_depth[frame_idx] = &VulkanRendererBase::m_gbuffer_depth[frame_idx];

		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Lighting data UBO */
		VkDescriptorBufferInfo info = { h_light_manager->m_ubo[frame_idx].buffer,  0, sizeof(LightData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		/* Depth g-buffer*/
		VkDescriptorImageInfo  descriptor_image_info = {};
		descriptor_image_info = { VulkanRendererBase::s_SamplerClampLinear, h_gbuffers_depth[frame_idx]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		desc_writes.push_back(ImageWriteDescriptorSet(m_descriptor_set[frame_idx], 1, &descriptor_image_info));

		/* Frame data UBO */
		VkDescriptorBufferInfo info2 = { VulkanRendererBase::m_ubo_framedata[frame_idx].buffer,  0, sizeof(VulkanRendererBase::PerFrameData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 2, &info2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void ShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { m_shadow_map_size.x , m_shadow_map_size.y } };
	set_viewport_scissor(cmd_buffer, m_shadow_map_size.x, m_shadow_map_size.y, true);
	m_shadow_pass[current_frame_idx].begin(cmd_buffer, render_area);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 3, 1, &VulkanRendererBase::m_framedata_desc_set[current_frame_idx].set, 0, nullptr);

	draw_scene(cmd_buffer);

	m_shadow_pass[current_frame_idx].end(cmd_buffer);

	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ShadowRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Directional Shadow Map Render Pass");

	for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	{
		const Drawable& drawable = RenderObjectManager::drawables[i];
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &drawable.mesh_handle->descriptor_set, 0, nullptr);

		/* Object descriptor set : per instance data */
		uint32_t dynamic_offset = drawable.id * RenderObjectManager::per_object_data_dynamic_aligment;
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 2, 1, &RenderObjectManager::drawable_descriptor_set, 1, &dynamic_offset);

		if (drawable.has_primitives)
		{
			for (int prim_idx = 0; prim_idx < drawable.mesh_handle->geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = drawable.mesh_handle->geometry_data.primitives[prim_idx];
				/* Model matrix push constant */
				glm::mat4 model_mat = drawable.transform.model * p.mat_model;
				vkCmdPushConstants(cmd_buffer, m_gfx_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model_mat);
				vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
			}
		}
		else
		{
			drawable.draw(cmd_buffer);
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Cascade
{
	float z_near;
	float z_far;
};

struct Plane
{
	glm::vec4 bottom_left;
	glm::vec4 bottom_right;
	glm::vec4 top_left;
	glm::vec4 top_right;

	Plane& transform(glm::mat4 matrix)
	{
		bottom_left		= matrix * bottom_left;
		bottom_right	= matrix * bottom_right;
		top_left		= matrix * top_left;
		top_right		= matrix * top_right;

		return *this;
	}

	glm::vec3 get_min()
	{
		glm::vec3 min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max() , std::numeric_limits<float>::max() };

		min.x = std::min(min.x, bottom_left.x);
		min.x = std::min(min.x, bottom_right.x);
		min.x = std::min(min.x, top_left.x);
		min.x = std::min(min.x, top_right.x);

		min.y = std::min(min.y, bottom_left.y);
		min.y = std::min(min.y, bottom_right.y);
		min.y = std::min(min.y, top_left.y);
		min.y = std::min(min.y, top_right.y);

		min.z = std::min(min.z, bottom_left.z);
		min.z = std::min(min.z, bottom_right.z);
		min.z = std::min(min.z, top_left.z);
		min.z = std::min(min.z, top_right.z);

		return min;
	}

	glm::vec3 get_max()
	{
		glm::vec3 max{ std::numeric_limits<float>::min(), std::numeric_limits<float>::min() , std::numeric_limits<float>::min() };

		max.x = std::max(max.x, bottom_left.x);
		max.x = std::max(max.x, bottom_right.x);
		max.x = std::max(max.x, top_left.x);
		max.x = std::max(max.x, top_right.x);

		max.y = std::max(max.y, bottom_left.y);
		max.y = std::max(max.y, bottom_right.y);
		max.y = std::max(max.y, top_left.y);
		max.y = std::max(max.y, top_right.y);

		max.z = std::max(max.z, bottom_left.z);
		max.z = std::max(max.z, bottom_right.z);
		max.z = std::max(max.z, top_left.z);
		max.z = std::max(max.z, top_right.z);

		return max;
	}
};

constexpr int num_cascades = 3;

void CascadedShadowRenderer::compute_cascade_ortho_proj()
{
	float n = h_camera->near_far.x;
	float f = h_camera->near_far.y;

	/* Compute cascade start and end points */
	std::array<Cascade, num_cascades> cascades;
	float cascade_length = (f - n) / num_cascades;
	cascades[0].z_near   = n;
	cascades[0].z_far   = cascades[0].z_near + cascade_length;
	for (size_t i = 1; i < cascades.size(); i++)
	{
		cascades[i].z_near = cascades[i - 1].z_far;
		cascades[i].z_far = cascades[i].z_near + cascade_length;
	}

	float ar = h_camera->aspect_ratio;

	/* Compute cascade corners in view space */
	float tanHalfFovX = tan(glm::radians((h_camera->fov * ar) / 2));
	float tanHalfFovY = tan(glm::radians(h_camera->fov / 2));

	glm::mat4 cam_inv_view_mat     = glm::inverse(h_camera->controller.m_view);
	glm::mat4 dir_light_view_mat   = h_light_manager->view;

	for (int i = 0; i < num_cascades; i++)
	{
		float xn { cascades[i].z_near * tanHalfFovX };
		float yn { cascades[i].z_near * tanHalfFovY };
		float xf { cascades[i].z_far * tanHalfFovX  };
		float yf { cascades[i].z_far * tanHalfFovY  };

		/* Calculate near and far planes for this cascade in view space */
		Plane near_plane_vs
		{
			.bottom_left   { -xn, -yn, cascades[i].z_near, 1.0f },
			.bottom_right  {  xn, -yn, cascades[i].z_near, 1.0f },
			.top_left      { -xn,  yn, cascades[i].z_near, 1.0f },
			.top_right     {  xn,  yn, cascades[i].z_near, 1.0f },
		};

		Plane far_plane_vs
		{
			.bottom_left   { -xf, -yf, cascades[i].z_far, 1.0f },
			.bottom_right  {  xf, -yf, cascades[i].z_far, 1.0f },
			.top_left      { -xf,  yf, cascades[i].z_far, 1.0f },
			.top_right     {  xf,  yf, cascades[i].z_far, 1.0f },
		};

		/* Calculate the bounding box in light space */
		near_plane_vs.transform(cam_inv_view_mat);   // World space
		near_plane_vs.transform(dir_light_view_mat); // Light space

		glm::vec3 min = near_plane_vs.get_min();
		glm::vec3 max = near_plane_vs.get_max();

		/* Orthographics projection */
		proj_matrices.push_back(glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z));
	}
}
