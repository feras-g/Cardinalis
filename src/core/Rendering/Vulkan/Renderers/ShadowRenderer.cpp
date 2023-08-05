#include "ShadowRenderer.h"
#include "Rendering/LightManager.h"
#include "Rendering/Vulkan/VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Camera.h"

static constexpr VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShadowRenderer::init(unsigned int width, unsigned int height, const LightManager& lightmanager)
{
	h_light_manager = &lightmanager;

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	m_shadow_map_size = { width, height };

	const char* attachment_debug_name = "Shadow Map";

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, width, height, 1, false);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, attachment_debug_name);
		m_shadow_maps[frame_idx].create_view(context.device, { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT });
		m_shadow_maps[frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
	desc_set_layouts.push_back(VulkanRendererBase::m_framedata_desc_set_layout.vk_set_layout);		/* Frame data */

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


	VkPushConstantRange pushConstantRanges[1] =
	{
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(glm::mat4)
		},
	};


	m_gfx_pipeline_layout   = create_pipeline_layout(context.device, desc_set_layouts, pushConstantRanges);

	/* Create graphics pipeline */
	m_shadow_shader.create("GenShadowMap.vert.spv", "GenShadowMap.frag.spv");
	Pipeline::Flags flags = Pipeline::Flags::ENABLE_DEPTH_STATE;
	Pipeline::create_graphics_pipeline_dynamic(m_shadow_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void ShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Lighting data UBO */
		VkDescriptorBufferInfo info = { h_light_manager->m_ubo[frame_idx],  0, sizeof(LightData) };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void ShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRect2D render_area{ .offset {}, .extent { m_shadow_map_size.x , m_shadow_map_size.y } };
	set_viewport_scissor(cmd_buffer, m_shadow_map_size.x, m_shadow_map_size.y, true);
	m_shadow_pass[current_frame_idx].begin(cmd_buffer, render_area);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 3, 1, &VulkanRendererBase::m_framedata_desc_set[current_frame_idx].vk_set, 0, nullptr);

	draw_scene(cmd_buffer);

	m_shadow_pass[current_frame_idx].end(cmd_buffer);

	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

		if (drawable.cast_shadows())
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

	const char* attachment_debug_name = "Cascaded Shadow Map";

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, m_shadow_map_size.x, m_shadow_map_size.y, (uint32_t)NUM_CASCADES, false);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, attachment_debug_name);
		m_shadow_maps[frame_idx].view = Texture2D::create_texture_2d_array_view(
			m_shadow_maps[frame_idx], m_shadow_maps[frame_idx].info.imageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		m_shadow_maps[frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		/* Store separate views */
		for (uint32_t cascade_idx = 0; cascade_idx < 4; cascade_idx++)
		{
			VkImageSubresourceRange subresource_desc 
			{
				.aspectMask     { VK_IMAGE_ASPECT_DEPTH_BIT },
				.baseMipLevel   { 0 },
				.levelCount     { 1 },
				.baseArrayLayer { cascade_idx },
				.layerCount     { 1 },
			};

			VkImageView view = create_texture_view(m_shadow_maps[frame_idx], m_shadow_maps[frame_idx].info.imageFormat, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, &subresource_desc);
			cascade_views[frame_idx][cascade_idx] = view;
		}

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

	VkPushConstantRange pushConstantRanges[1] =
	{
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(push_constants)
		},
	};

	m_gfx_pipeline_layout = create_pipeline_layout(context.device, desc_set_layouts, pushConstantRanges);

	create_buffers();
	compute_cascade_splits();
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		compute_cascade_ortho_proj(frame_idx);
	}

	update_desc_sets();

	/* Create graphics pipeline */
	m_csm_shader.create("GenCascadedShadowMap.vert.spv", "GenShadowMap.frag.spv");
	Pipeline::Flags flags = Pipeline::Flags::ENABLE_DEPTH_STATE;
	Pipeline::create_graphics_pipeline_dynamic(m_csm_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, {}, view_mask);
}

void CascadedShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		std::vector<VkWriteDescriptorSet> desc_writes = {};

		/* Cascades projection matrices UBO */
		VkDescriptorBufferInfo info = { view_proj_mats_ubo[frame_idx],  0, mats_ubo_size_bytes };
		desc_writes.push_back(BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));

		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void CascadedShadowRenderer::create_buffers()
{
	mats_ubo_size_bytes = sizeof(glm::mat4) * NUM_CASCADES;
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		view_proj_mats_ubo[frame_idx].init(Buffer::Type::UNIFORM, mats_ubo_size_bytes);
	}
	cascade_splits_ubo_size = sizeof(float) * cascade_splits.size();
	cascade_ends_ubo.init(Buffer::Type::UNIFORM, cascade_splits_ubo_size);
}

void CascadedShadowRenderer::compute_cascade_splits()
{
	// Use Practical Split Scheme method
	// https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf, page 6
	
	near_clip  = h_camera->near_far.x;
	far_clip   = h_camera->near_far.y;
	clip_range = far_clip - near_clip;

	float z_min = near_clip;
	float z_max = near_clip + clip_range;

	float range = z_max - z_min;
	float ratio = z_max / z_min;

	float prev_split_dist = 0.0f;

	for(int i = 0; i < NUM_CASCADES; ++i)
	{
		float p = (i + 1) / (float)(NUM_CASCADES);
		float log = z_min * std::pow(ratio, p);
		float uniform = z_min + range * p;
		float d = lambda * (log - uniform) + uniform;
		cascade_splits[i] = (d - near_clip) / clip_range;

		float split_dist = cascade_splits[i];

		per_cascade_projection[i] = glm::perspective(glm::radians(h_camera->fov), h_camera->aspect_ratio, (near_clip + prev_split_dist * clip_range), (near_clip + split_dist * clip_range));
		prev_split_dist = split_dist;
	}
	
}

void CascadedShadowRenderer::compute_cascade_ortho_proj(size_t frame_idx)
{
	compute_cascade_splits();

	glm::vec3 light_direction = glm::normalize(LightManager::direction);
	
	glm::vec4 cascade_splits_view_space_depth;

	static constexpr glm::vec3 up     = { 0.0f, 1.0f, 0.0f };

	glm::mat4 cam_view = h_camera->controller.m_view;
	glm::mat4 cam_proj = h_camera->projection;

	/* Compute orthographic matrix for each cascade */
	float prev_split_dist = 0.0f;
	for (int cascade_idx = 0; cascade_idx < NUM_CASCADES; cascade_idx++)
	{
		float split_dist = cascade_splits[cascade_idx];

		/* Get the corners of the view frustum in world space */

		/* The frustum in NDC is a cube */
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


		/* Transform to World Space : Cube becomes a frustum in World Space */
		glm::mat4 view_proj_inv = glm::inverse(h_camera->projection * cam_view);

		for (int corner_idx = 0; corner_idx < 8; corner_idx++)
		{
			glm::vec4 corner = view_proj_inv * glm::vec4(frustum_corners[corner_idx], 1.0f);
			frustum_corners[corner_idx] = corner / corner.w;
		}

		/* Get the corners of the current cascade */
		for (int corner_idx = 0; corner_idx < 4; corner_idx++) 
		{
			glm::vec3 dist = frustum_corners[corner_idx + 4] - frustum_corners[corner_idx];
			frustum_corners[corner_idx + 4] = frustum_corners[corner_idx] + (dist * split_dist);
			frustum_corners[corner_idx] = frustum_corners[corner_idx] + (dist * prev_split_dist);
		}

		/* Get frustum center in World space */
		glm::vec3 frustum_center = glm::vec3(0);
		for (int corner_idx = 0; corner_idx < 8; corner_idx++)
		{
			frustum_center += frustum_corners[corner_idx];
		}
		frustum_center /= 8.0f;

		/* Compute AABB around frustum */
//		glm::vec3 up = glm::normalize(h_camera->controller.m_up);
//		glm::vec3 lightCameraPos = frustum_center;
//		glm::vec3 lookAt = frustum_center - light_direction;
//		glm::mat4x4 lightView = glm::lookAt(lightCameraPos, lookAt, up);
//
//		glm::vec3 mins  = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
//		glm::vec3 maxes = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
//
//		for(int i = 0; i < 8; ++i)
//		{
//			glm::vec3 corner = glm::vec3(lightView * glm::vec4(frustum_corners[i], 1.0f));
//			mins  = glm::min(mins, corner);
//			maxes = glm::max(maxes, corner);
//		}
//
//		glm::vec3 minExtents = mins;
//		glm::vec3 maxExtents = maxes;
//
//		// Adjust the min/max to accommodate the filtering size
//		float scale = (m_shadow_map_size.x  + 9.0f) / float(m_shadow_map_size.x);
//		minExtents.x *= scale;
//		minExtents.y *= scale;
//		maxExtents.x *= scale;
//		maxExtents.y *= scale;


		float cascade_radius = 0.0f;
		for (int corner_idx = 0; corner_idx < 8; corner_idx++)
		{
			cascade_radius = std::max(cascade_radius, glm::length(frustum_center - frustum_corners[corner_idx]));
		}
		cascade_radius = std::ceil(cascade_radius * 16.0f) / 16.0f;
		glm::vec3 maxExtents = glm::vec3(cascade_radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 cascade_extents = maxExtents - minExtents;

		glm::mat4 lightViewMatrix  = glm::lookAt(frustum_center - light_direction * -minExtents.z, frustum_center, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		cascade_splits_view_space_depth[cascade_idx] = (near_clip + split_dist * clip_range) * -1.0f;
		view_proj_mats[cascade_idx] = lightOrthoMatrix * lightViewMatrix;
		prev_split_dist = cascade_splits[cascade_idx];
	}
	view_proj_mats_ubo[frame_idx].upload(context.device, view_proj_mats.data(), 0, mats_ubo_size_bytes);
	cascade_ends_ubo.upload(context.device, &cascade_splits_view_space_depth, 0, cascade_splits_ubo_size);
}

void CascadedShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	compute_cascade_ortho_proj(current_frame_idx);

	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	VkRect2D render_area{ .offset {}, .extent { (uint32_t)m_shadow_map_size.x , (uint32_t)m_shadow_map_size.y } };
	set_viewport_scissor(cmd_buffer, (uint32_t)m_shadow_map_size.x, (uint32_t)m_shadow_map_size.y, true);

	m_CSM_pass[current_frame_idx].begin(cmd_buffer, render_area, view_mask);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);

	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);

	/* Push constant */
	draw_scene(cmd_buffer);

	m_CSM_pass[current_frame_idx].end(cmd_buffer);

	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}

void CascadedShadowRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Cascaded Shadow Map Render Pass");

	for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	{
		const Drawable& drawable = RenderObjectManager::drawables[i];
		const VulkanMesh& mesh = drawable.get_mesh();
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &mesh.descriptor_set, 0, nullptr);
		
		if (drawable.cast_shadows())
		{
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
}
