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

	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(m_descriptor_set_layout);								/* Shadow pass descriptor set */
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);		/* Mesh geometry descriptor set*/
	desc_set_layouts.push_back(RenderObjectManager::drawable_descriptor_set_layout);	/* Drawable data descriptor set */
	desc_set_layouts.push_back(VulkanRendererBase::m_framedata_desc_set_layout.layout);		/* Frame data */

	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
	};
	m_descriptor_pool       = create_descriptor_pool(pool_sizes, NUM_FRAMES);
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_descriptor_set[frame_idx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
	}

	update_desc_sets();

	size_t vtx_pushconstantsize = sizeof(glm::mat4);
	m_gfx_pipeline_layout   = create_pipeline_layout(context.device, desc_set_layouts, (uint32_t)vtx_pushconstantsize, 0);

	/* Create graphics pipeline */
	m_shadow_shader.create("GenShadowMap.vert.spv", "GenShadowMap.frag.spv");
	GfxPipeline::Flags flags = GfxPipeline::Flags::ENABLE_DEPTH_STATE;
	GfxPipeline::CreateDynamic(m_shadow_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void ShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Lighting data UBO */
		VkDescriptorBufferInfo info = { h_light_manager->m_ubo[frame_idx].buffer,  0, sizeof(LightData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

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
		const VulkanMesh& mesh = drawable.get_mesh();
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &mesh.descriptor_set, 0, nullptr);

		/* Object descriptor set : per instance data */
		uint32_t dynamic_offset = static_cast<uint32_t>(drawable.id * RenderObjectManager::per_object_data_dynamic_aligment);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 
		                        2, 1, &RenderObjectManager::drawable_descriptor_set, 1, &dynamic_offset);

		if (drawable.visible)
		{
			if (drawable.has_primitives())
			{
				for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
				{
					const Primitive& p = mesh.geometry_data.primitives[prim_idx];
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

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CascadedShadowRenderer::init(unsigned int width, unsigned int height,  Camera& camera, const LightManager& lightmanager)
{
	h_light_manager = &lightmanager;
	h_camera = &camera;

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	m_shadow_map_size = { width, height };

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, m_shadow_map_size.x, m_shadow_map_size.y, (uint32_t)NUM_CASCADES, false);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_shadow_maps[frame_idx].view = Texture2D::create_texture_array_view(m_shadow_maps[frame_idx]); //create_view(context.device, { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT });
		m_shadow_maps[frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* Create render pass */
		m_CSM_pass[frame_idx].add_depth_attachment(m_shadow_maps[frame_idx].view);
	}

	end_temp_cmd_buffer(cmd_buffer);

	/* Shadow pass set */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }); /* Lighting data */
	bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }); /* Cascades view-projection matrices */
	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);			/* Mesh geometry descriptor set*/
	desc_set_layouts.push_back(m_descriptor_set_layout);									/* Shadow pass descriptor set */


	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
	};
	m_descriptor_pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_descriptor_set[frame_idx] = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
	}

	size_t vtx_pushconstantsize = sizeof(glm::mat4);
	m_gfx_pipeline_layout = create_pipeline_layout(context.device, desc_set_layouts, (uint32_t)sizeof(push_constants), 0);

	create_buffers();
	compute_z_splits();
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		compute_cascade_ortho_proj(frame_idx);
	}

	update_desc_sets();

	/* Create graphics pipeline */
	m_csm_shader.create("GenCascadedShadowMap.vert.spv", "GenShadowMap.frag.spv");
	GfxPipeline::Flags flags = GfxPipeline::Flags::ENABLE_DEPTH_STATE;
	GfxPipeline::CreateDynamic(m_csm_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, {}, view_mask);
}

void CascadedShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Cascades projection matrices UBO */
		VkDescriptorBufferInfo info = { view_proj_mats_ubo[frame_idx].buffer,  0, mats_ubo_size_bytes };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void CascadedShadowRenderer::create_buffers()
{
	mats_ubo_size_bytes = sizeof(glm::mat4) * NUM_CASCADES;
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		create_buffer(Buffer::Type::UNIFORM, view_proj_mats_ubo[frame_idx], mats_ubo_size_bytes);
	}
	cascade_ends_ubo_size_bytes = sizeof(glm::vec4);
	create_buffer(Buffer::Type::UNIFORM, cascade_ends_ubo, cascade_ends_ubo_size_bytes);
}

CascadedShadowRenderer::~CascadedShadowRenderer()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		destroy_buffer(view_proj_mats_ubo[frame_idx]);
	}
	destroy_buffer(cascade_ends_ubo);
}

void CascadedShadowRenderer::compute_z_splits()
{
	// https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf, Page 6
	//  compute the z-values of the splits of the view frustum in camera eye space
	
	const float n = frustum_near;
	const float f = frustum_far;

	const float range = f - n;
	const float ratio = f / n;

	// interpolation factor between exponential and uniform distribution of splits
	glm::vec4 z_splits_view_space;

	for (int i = 0; i < NUM_CASCADES; i++)
	{
		float p = (float)(i + 1) / NUM_CASCADES;
		float log_distr     = n * std::pow(ratio, p); // exponential distribution of splits
		float uniform_distr = n + p * range; // uniform distribution of splits

		z_splits[i] = std::lerp(uniform_distr, log_distr, interp_factor);
		z_splits_view_space[i] = z_splits[i];

		/* Optional: compute the projection matrix corresponding to this cascade */
		float n = (i > 0) ? z_splits[i - 1] : frustum_near;
		camera_splits_proj_mats[i] = glm::perspective(glm::radians(h_camera->fov), h_camera->aspect_ratio, n, z_splits[i]);
	}

	upload_buffer_data(cascade_ends_ubo, &z_splits_view_space, cascade_ends_ubo_size_bytes, 0);
}

void CascadedShadowRenderer::compute_cascade_ortho_proj(size_t frame_idx)
{
	glm::vec3 light_direction = glm::normalize(LightManager::direction);

	static constexpr glm::vec3 up     = { 0.0f, 1.0f, 0.0f };

	glm::mat4 cam_view = h_camera->controller.m_view;

	/* Compute orthographic matrix for each cascade */
	for (int i = 0; i < NUM_CASCADES; i++)
	{
		/* Frustum corners in NDC */
		glm::vec3 frustum_corners[8] = 
		{
			glm::vec3(-1.0f,  1.0f, 0.0f),	
			glm::vec3( 1.0f,  1.0f, 0.0f),	
			glm::vec3( 1.0f, -1.0f, 0.0f),	
			glm::vec3(-1.0f, -1.0f, 0.0f),	
			glm::vec3(-1.0f,  1.0f, 1.0f),	
			glm::vec3( 1.0f,  1.0f, 1.0f),
			glm::vec3( 1.0f, -1.0f, 1.0f),
			glm::vec3(-1.0f, -1.0f, 1.0f),
		};

		/* NDC -> World Space : Cube becomes a frustum in World Space */
		glm::mat4 inv_vp = glm::inverse(camera_splits_proj_mats[i] * cam_view);
		for (size_t i = 0; i < 8; i++)
		{
			glm::vec4 corner = inv_vp * glm::vec4(frustum_corners[i], 1.0f);
			frustum_corners[i] = corner / corner.w;
		}

		/* Get frustum center in World space */
		glm::vec3 frustum_center = glm::vec3(0);
		
		for (int i = 0; i < 8; i++)
		{
			frustum_center += frustum_corners[i];
		}
		frustum_center /= 8.0f;

		float cascade_radius = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			cascade_radius = std::max(cascade_radius, glm::length(frustum_center - frustum_corners[i]));
		}

		/* Remove shimmering */
		{
			float texelsPerUnit = (float)m_shadow_map_size.x / (cascade_radius * 2.0f);
			glm::mat4 scalar = glm::scale(glm::identity<glm::mat4>(), glm::vec3(texelsPerUnit));

			glm::vec3 zero(0.0f);
			glm::mat4 lookAt, lookAtInv;
			glm::vec3 baseLookAt(light_direction);

			lookAt = glm::lookAt(zero, baseLookAt, up);
			lookAt = scalar * lookAt;
			lookAtInv = glm::inverse(lookAt);

			frustum_center = glm::vec3(lookAt * glm::vec4(frustum_center, 1.0f));
			frustum_center.x = (float)std::floor(frustum_center.x);
			frustum_center.y = (float)std::floor(frustum_center.y);
			frustum_center = glm::vec3(lookAtInv * glm::vec4(frustum_center, 1.0f));
		}

		/* Orthographics projection */
		glm::vec3 eye = frustum_center + (-light_direction * cascade_radius * 2.0f);
		glm::mat4 light_view = glm::lookAt(eye, frustum_center, up);
		view_proj_mats[i] = glm::ortho(-cascade_radius, cascade_radius, -cascade_radius, cascade_radius, -cascade_radius * 6.0f, 6.0f * cascade_radius) * light_view;
	}
	upload_buffer_data(view_proj_mats_ubo[frame_idx], view_proj_mats.data(), mats_ubo_size_bytes, 0);
}

void CascadedShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	compute_cascade_ortho_proj(current_frame_idx);

	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { (uint32_t)m_shadow_map_size.x , (uint32_t)m_shadow_map_size.y } };
	set_viewport_scissor(cmd_buffer, (uint32_t)m_shadow_map_size.x, (uint32_t)m_shadow_map_size.y, true);

	m_CSM_pass[current_frame_idx].begin(cmd_buffer, render_area, view_mask);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);

	/* Push constant */

	draw_scene(cmd_buffer);

	m_CSM_pass[current_frame_idx].end(cmd_buffer);

	m_shadow_maps[current_frame_idx].transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void CascadedShadowRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Cascaded Shadow Map Render Pass");

	for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	{
		const Drawable& drawable = RenderObjectManager::drawables[i];
		const VulkanMesh& mesh = drawable.get_mesh();
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &mesh.descriptor_set, 0, nullptr);
		
		if (drawable.has_primitives())
		{
			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
			{
				const Primitive& p = mesh.geometry_data.primitives[prim_idx];
				/* Model matrix push constant */
				ps.model = drawable.transform.model * p.mat_model;
				ps.dir_light_view = LightManager::view;

				vkCmdPushConstants(cmd_buffer, m_gfx_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &ps);
				vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
			}
		}
		else
		{
			drawable.draw(cmd_buffer);
		}
	}
}
