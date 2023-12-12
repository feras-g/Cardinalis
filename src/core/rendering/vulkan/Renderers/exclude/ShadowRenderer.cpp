#include "ShadowRenderer.h"
#include "core/rendering/vulkan/LightManager.h"
#include "core/rendering/vulkan/VulkanMesh.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/rendering/vulkan/VulkanDebugUtils.h"
#include "core/rendering/Camera.h"

#include "glm/gtx/quaternion.hpp"

static constexpr VkFormat shadow_map_format = VK_FORMAT_D32_SFLOAT;

void CascadedShadowRenderer::init(unsigned int width, unsigned int height,  Camera& camera, const LightManager& lightmanager)
{
	h_light_manager = &lightmanager;
	h_camera = &camera;

	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	m_shadow_map_size = { width, height };

	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		std::string attachment_debug_name = "Cascaded Shadow Map Frame#" + std::to_string(frame_idx);
		/* Create shadow map */
		m_shadow_maps[frame_idx].init(shadow_map_format, m_shadow_map_size.x, m_shadow_map_size.y, (uint32_t)NUM_CASCADES, false, attachment_debug_name);
		m_shadow_maps[frame_idx].create(context.device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_shadow_maps[frame_idx].view = Texture2D::create_texture_2d_array_view(
			m_shadow_maps[frame_idx], m_shadow_maps[frame_idx].info.imageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		m_shadow_maps[frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

		/* Create render pass */
		m_renderpass[frame_idx].add_depth_attachment(m_shadow_maps[frame_idx].view);
		
		/* Store separate views */
		for (uint32_t cascade_idx = 0; cascade_idx < (uint32_t)NUM_CASCADES; cascade_idx++)
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


	}

	end_temp_cmd_buffer(cmd_buffer);

	/* Shadow pass set */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }); /* Cascades view-projection matrices */
	m_descriptor_set_layout = create_descriptor_set_layout(bindings);

	std::vector<VkDescriptorSetLayout> desc_set_layouts = {};
	//desc_set_layouts.push_back(RenderObjectManager::mesh_descriptor_set_layout);			/* Mesh geometry descriptor set*/
	desc_set_layouts.push_back(m_descriptor_set_layout);									/* Shadow pass descriptor set */

	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
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
	//Pipeline::create_graphics_pipeline_dynamic(m_csm_shader, {}, shadow_map_format, flags, m_gfx_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, {}, view_mask);
}

void CascadedShadowRenderer::update_desc_sets()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		VkDescriptorBufferInfo info = { cascades_ssbo[frame_idx],  0, cascades_ssbo_size_bytes };
		std::vector<VkWriteDescriptorSet> desc_writes = 
		{
			BufferWriteDescriptorSet(m_descriptor_set[frame_idx], 0, info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		};
		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
	}
}

void CascadedShadowRenderer::create_buffers()
{
	cascades_ssbo_size_bytes = sizeof(CascadesData);
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		cascades_ssbo[frame_idx].init(Buffer::Type::STORAGE, cascades_ssbo_size_bytes, "CascadedShowRenderer SSBO");
	}
}

void CascadedShadowRenderer::compute_cascade_splits()
{
	// Use Practical Split Scheme method
	// https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf, page 6
	
	near_clip  = h_camera->znear;
	far_clip   = h_camera->zfar;
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

	glm::vec3 light_direction = h_light_manager->light_data.directional_light.direction;

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

		float cascade_radius = 0.0f;
		for (int corner_idx = 0; corner_idx < 8; corner_idx++)
		{
			cascade_radius = std::max(cascade_radius, glm::length(frustum_center - frustum_corners[corner_idx]));
		}
		cascade_radius = std::ceil(cascade_radius * 16.0f) / 16.0f;
		glm::vec3 maxExtents = glm::vec3(cascade_radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 cascade_extents = maxExtents - minExtents;

		glm::mat4 lightViewMatrix  = glm::lookAt(frustum_center - light_direction * -minExtents.z, frustum_center, glm::vec3(0.0f, -1.0f, 0.0f));
		glm::mat4 lightProjMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);


		/* Rounding matrix */
		glm::mat4 light_view_proj = lightProjMatrix * lightViewMatrix;
		glm::vec4 shadow_origin { 0.0f, 0.0f, 0.0f, 1.0f };
		shadow_origin = light_view_proj * shadow_origin;
		shadow_origin = shadow_origin * (m_shadow_map_size.x * 0.5f);

		glm::vec4 rounded_shadow_origin = glm::round(shadow_origin);
		glm::vec4 round_offset = rounded_shadow_origin - shadow_origin;

		round_offset = round_offset * (2.0f / m_shadow_map_size.x);
		round_offset.z = 0.0f;
		round_offset.w = 0.0f;
		lightProjMatrix[3] += round_offset;


		// Store split distance and matrix in cascade
		cascades_data.num_cascades.x = NUM_CASCADES;
		cascades_data.splits[cascade_idx] = (near_clip + split_dist * clip_range) * -1.0f;
		cascades_data.view_proj[cascade_idx] = lightProjMatrix * lightViewMatrix;
		prev_split_dist = cascade_splits[cascade_idx];
	}

	cascades_ssbo[frame_idx].upload(context.device, &cascades_data, 0, cascades_ssbo_size_bytes);
}

void CascadedShadowRenderer::render(size_t current_frame_idx, VkCommandBuffer cmd_buffer)
{
	compute_cascade_ortho_proj(current_frame_idx);


	set_viewport_scissor(cmd_buffer, (uint32_t)m_shadow_map_size.x, (uint32_t)m_shadow_map_size.y, true);

	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	m_renderpass[current_frame_idx].begin(cmd_buffer, { (uint32_t)m_shadow_map_size.x , (uint32_t)m_shadow_map_size.y }, view_mask);

	/* Bind pipeline */
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 1, 1, &m_descriptor_set[current_frame_idx], 0, nullptr);

	/* Push constant */
	draw_scene(cmd_buffer);

	m_renderpass[current_frame_idx].end(cmd_buffer);
	m_shadow_maps[current_frame_idx].transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
}

void CascadedShadowRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	VULKAN_RENDER_DEBUG_MARKER(cmd_buffer, "Cascaded Shadow Map Render Pass");

	//for (size_t i = 0; i < RenderObjectManager::drawables.size(); i++)
	//{
		//const Drawable& drawable = RenderObjectManager::drawables[i];
		//const VulkanMesh& mesh = drawable.get_mesh();
		//vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline_layout, 0, 1, &mesh.descriptor_set, 0, nullptr);
		
	//	if (drawable.cast_shadows())
	//	{
	//		if (drawable.has_primitives())
	//		{
	//			for (int prim_idx = 0; prim_idx < mesh.geometry_data.primitives.size(); prim_idx++)
	//			{
	//				const Primitive& p = mesh.geometry_data.primitives[prim_idx];
	//				/* Model matrix push constant */
	//				ps.model = drawable.transform.model * p.mat_model;

	//				vkCmdPushConstants(cmd_buffer, m_gfx_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &ps);
	//				vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
	//			}
	//		}
	//		else
	//		{
	//			drawable.draw(cmd_buffer);
	//		}
	//	}

	//}
}

//void CascadedShadowRenderer::compute_cascade_ortho_proj(size_t frame_idx)
//{
//	glm::vec4 cascade_splits_view_space_depth;
//
//	constexpr int SHADOW_MAP_CASCADE_COUNT = 4;
//	float cascadeSplitLambda = 0.95f;
//
//	float cascadeSplits[4];
//
//	float nearClip = 0.5f;
//	float farClip =  48.0f;
//	float clipRange = farClip - nearClip;
//
//	float minZ = nearClip;
//	float maxZ = nearClip + clipRange;
//
//	float range = maxZ - minZ;
//	float ratio = maxZ / minZ;
//
//	// Calculate split depths based on view camera frustum
//	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
//	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
//		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
//		float log = minZ * std::pow(ratio, p);
//		float uniform = minZ + range * p;
//		float d = cascadeSplitLambda * (log - uniform) + uniform;
//		cascadeSplits[i] = (d - nearClip) / clipRange;
//	}
//	// Calculate orthographic projection matrix for each cascade
//	float lastSplitDist = 0.0;
//	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
//		float splitDist = cascadeSplits[i];
//
//		glm::vec3 frustumCorners[8] = {
//			glm::vec3(-1.0f,  1.0f, 0.0f),
//			glm::vec3(1.0f,  1.0f, 0.0f),
//			glm::vec3(1.0f, -1.0f, 0.0f),
//			glm::vec3(-1.0f, -1.0f, 0.0f),
//			glm::vec3(-1.0f,  1.0f,  1.0f),
//			glm::vec3(1.0f,  1.0f,  1.0f),
//			glm::vec3(1.0f, -1.0f,  1.0f),
//			glm::vec3(-1.0f, -1.0f,  1.0f),
//		};
//
//		// Project frustum corners into world space
//		
//		glm::mat4 invCam = glm::inverse(h_camera->GetProj() * h_camera->GetView());
//		for (uint32_t i = 0; i < 8; i++) {
//			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
//			frustumCorners[i] = invCorner / invCorner.w;
//		}
//
//		for (uint32_t i = 0; i < 4; i++) {
//			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
//			frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
//			frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
//		}
//
//		// Get frustum center
//		glm::vec3 frustumCenter = glm::vec3(0.0f);
//		for (uint32_t i = 0; i < 8; i++) {
//			frustumCenter += frustumCorners[i];
//		}
//		frustumCenter /= 8.0f;
//
//		float radius = 0.0f;
//		for (uint32_t i = 0; i < 8; i++) {
//			float distance = glm::length(frustumCorners[i] - frustumCenter);
//			radius = glm::max(radius, distance);
//		}
//		radius = std::ceil(radius * 16.0f) / 16.0f;
//
//		glm::vec3 maxExtents = glm::vec3(radius);
//		glm::vec3 minExtents = -maxExtents;
//
//		glm::vec3 lightDir = glm::normalize(h_light_manager->light_data.directional_light.direction);
//		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
//		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
//
//		// Store split distance and matrix in cascade
//		cascade_splits_view_space_depth[i] = (nearClip + splitDist * clipRange) * -1.0f;
//		view_proj_mats[i] = lightOrthoMatrix * lightViewMatrix;
//
//		lastSplitDist = cascadeSplits[i];
//	}
//	view_proj_mats_ubo[frame_idx].upload(context.device, view_proj_mats.data(), 0, mats_ubo_size_bytes);
//	cascade_ends_ubo.upload(context.device, &cascade_splits_view_space_depth, 0, cascade_splits_ubo_size);
//}