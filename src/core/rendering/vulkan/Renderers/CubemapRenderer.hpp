#pragma once

#include "IRenderer.h"

/* Renders a cubemap from a spherical environement map */

struct CubemapRenderer
{
	void init(Texture2D& spherical_env_map)
	{
		/* Resources */
		write_cubemap_shader.create("write_cubemap_vert.vert.spv", "write_cubemap_frag.frag.spv");
		layer_size = std::min(spherical_env_map.info.width, spherical_env_map.info.height);
		cubemap_attachment.init(spherical_env_map.info.imageFormat, layer_size, layer_size, 6, false, "Cubemap Texture");
		cubemap_attachment.create_vk_image(context.device, true, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		cubemap_attachment.create_view(context.device, ImageViewTextureCubemap);

		VkSampler& sampler = VulkanRendererCommon::get_instance().s_SamplerClampNearest; 

		for (int layer = 0; layer < 6; layer++)
		{
			Texture2D::create_texture_2d_layer_view(cubemap_layer_view[layer], cubemap_attachment, context.device, layer);
			cubemap_layer_view_ui_id[layer] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler, cubemap_layer_view[layer], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		mesh_skybox.create_from_file("basic/skybox.glb");
		mesh_skybox_id = ObjectManager::get_instance().add_mesh(mesh_skybox, "Mesh_Skybox", {});

		ubo_cube_matrix_data.init(Buffer::Type::UNIFORM, sizeof(CubeMatrixData), "UBO Cube Matrix Data");
		ubo_cube_matrix_data.create();
		ubo_cube_matrix_data.upload(context.device, &cube_matrix_data, 0, sizeof(CubeMatrixData));

		VkDescriptorPoolSize pool_sizes[2]
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		descriptor_pool = create_descriptor_pool(pool_sizes, 1);

		cubemap_descriptor_set.layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "Cube Matrix Data binding");
		cubemap_descriptor_set.layout.add_combined_image_sampler_binding(1, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &sampler, "Spherical Env Map binding");
		cubemap_descriptor_set.layout.create("");
		cubemap_descriptor_set.create(descriptor_pool, "");

		cubemap_descriptor_set.write_descriptor_uniform_buffer(0, ubo_cube_matrix_data, 0, VK_WHOLE_SIZE);
		cubemap_descriptor_set.write_descriptor_combined_image_sampler(1, spherical_env_map.view, sampler);

		renderpass.add_color_attachment(cubemap_attachment.view);

		VkDescriptorSetLayout set_layouts[]
		{
			ObjectManager::get_instance().mesh_descriptor_set_layout,
			VulkanRendererCommon::get_instance().m_framedata_desc_set_layout,
			cubemap_descriptor_set.layout

		};
		pipeline.layout.create(set_layouts);
		VkFormat color_formats[] = { cubemap_attachment.info.imageFormat };
		pipeline.create_graphics(write_cubemap_shader, color_formats, VK_FORMAT_UNDEFINED, Pipeline::Flags::NONE, pipeline.layout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, multiview_mask);
	}

	void render()
	{
		VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();
		set_viewport_scissor(cmd_buffer, cubemap_attachment.info.width, cubemap_attachment.info.height);

		pipeline.bind(cmd_buffer);
		VkDescriptorSet descriptor_sets[]
		{
			ObjectManager::get_instance().m_descriptor_sets[mesh_skybox_id],
			VulkanRendererCommon::get_instance().m_framedata_desc_set[context.curr_frame_idx],
			cubemap_descriptor_set,
		};
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 3, descriptor_sets, 0, nullptr);
		cubemap_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		renderpass.begin(cmd_buffer, { cubemap_attachment.info.width, cubemap_attachment.info.height }, multiview_mask);
		const ObjectManager& object_manager = ObjectManager::get_instance();
		for (int prim_idx = 0; prim_idx < object_manager.m_meshes[mesh_skybox_id].geometry_data.primitives.size(); prim_idx++)
		{
			const Primitive& p = object_manager.m_meshes[mesh_skybox_id].geometry_data.primitives[prim_idx];
			vkCmdDraw(cmd_buffer, p.vertex_count, 1, p.first_vertex, 0);
		}
		renderpass.end(cmd_buffer);
		cubemap_attachment.transition(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		end_temp_cmd_buffer(cmd_buffer);
	}

	struct CubeMatrixData
	{
		/* View matrices for each cube face */
		glm::vec4 colors[6]
		{
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
			glm::vec4(0.0f, 0.0f, -1.0f, 1.0f),
		};

		
		glm::mat4 view_matrices[6]
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f,-1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		};
		glm::mat4 model = glm::identity<glm::mat4>();
		glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	} cube_matrix_data;

	void show_ui()
	{
		const ImVec2 thumbnail_size { 128, 128 };
		if (ImGui::Begin("Cubemap Faces"))
		{
			for (int i = 0; i < 6; i++)
			{
				ImGui::Text(ui_image_names[i]);
				ImGui::Image(cubemap_layer_view_ui_id[i], thumbnail_size);
			}
		}
		ImGui::End();
	}

	const char* ui_image_names[6] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

	ImTextureID cubemap_layer_view_ui_id[6];
	VkImageView cubemap_layer_view[6];

	Buffer ubo_cube_matrix_data;
	DescriptorSet cubemap_descriptor_set;
	VkDescriptorPool descriptor_pool;
	VulkanRenderPassDynamic renderpass;
	VertexFragmentShader write_cubemap_shader;
	Texture2D cubemap_attachment;
	uint32_t layer_size;
	Pipeline pipeline;
	const uint32_t multiview_mask = 0b00111111;
	VulkanMesh mesh_skybox;
	size_t mesh_skybox_id;
};