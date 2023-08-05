#include "VulkanRendererBase.h"

VkSampler VulkanRendererBase::s_SamplerRepeatLinear;
VkSampler VulkanRendererBase::s_SamplerClampLinear;
VkSampler VulkanRendererBase::s_SamplerClampNearest;
VkSampler VulkanRendererBase::s_SamplerRepeatNearest;

/** Size in pixels of the offscreen buffers */
uint32_t VulkanRendererBase::render_width  = 2048;
uint32_t VulkanRendererBase::render_height = 2048;

/* Formats */
VkFormat 			VulkanRendererBase::swapchain_color_format 			= VK_FORMAT_B8G8R8A8_SRGB;
VkColorSpaceKHR 	VulkanRendererBase::swapchain_colorspace 			= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
VkFormat 			VulkanRendererBase::swapchain_depth_format 			= VK_FORMAT_D32_SFLOAT;
VkFormat 			VulkanRendererBase::tex_base_color_format         	= VK_FORMAT_R8G8B8A8_SRGB;
VkFormat 			VulkanRendererBase::tex_metallic_roughness_format 	= VK_FORMAT_R8G8B8A8_UNORM;
VkFormat 			VulkanRendererBase::tex_normal_map_format         	= VK_FORMAT_R8G8B8A8_UNORM;
VkFormat 			VulkanRendererBase::tex_emissive_format           	= VK_FORMAT_R8G8B8A8_SRGB;
VkFormat 			VulkanRendererBase::tex_gbuffer_normal_format     	= VK_FORMAT_R16G16B16A16_SFLOAT;
VkFormat 			VulkanRendererBase::tex_gbuffer_depth_format 		= VK_FORMAT_D16_UNORM;
VkFormat 			VulkanRendererBase::tex_deferred_lighting_format 	= VK_FORMAT_R8G8B8A8_SRGB;
	
void VulkanRendererBase::create_descriptor_sets()
{
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
	};

	VkDescriptorPool pool = create_descriptor_pool(pool_sizes, NUM_FRAMES);
	m_framedata_desc_set_layout.add_uniform_buffer_binding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "Framedata UBO");
	m_framedata_desc_set_layout.create("Framedata layout");

	/* Frame data UBO */
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_framedata_desc_set[frame_idx].assign_layout(m_framedata_desc_set_layout);
		m_framedata_desc_set[frame_idx].create(pool, "Framedata descriptor set");
		
		VkDescriptorBufferInfo info = { m_ubo_framedata[frame_idx],  0, sizeof(VulkanRendererBase::PerFrameData) };
		VkWriteDescriptorSet write = BufferWriteDescriptorSet(m_framedata_desc_set[frame_idx].vk_set, 0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
	}
}

void VulkanRendererBase::create_samplers()
{
	create_sampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatLinear);
	create_sampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, s_SamplerRepeatNearest);
	create_sampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampLinear);
	create_sampler(context.device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, s_SamplerClampNearest);
}

void VulkanRendererBase::create_buffers()
{
	for (size_t frame_idx = 0; frame_idx < NUM_FRAMES; frame_idx++)
	{
		m_ubo_framedata[frame_idx].init(Buffer::Type::UNIFORM, sizeof(PerFrameData));
	}
}

void VulkanRendererBase::update_frame_data(const PerFrameData& data, size_t current_frame_idx)
{
	m_ubo_framedata[current_frame_idx].upload(context.device, (void*)&data, 0, sizeof(data));
}

VulkanRendererBase::~VulkanRendererBase()
{
	//vkDeviceWaitIdle(context.device);
	//vkDestroySampler(context.device, s_SamplerRepeatLinear, nullptr);
	//vkDestroySampler(context.device, s_SamplerClampLinear, nullptr);
	//vkDestroySampler(context.device, s_SamplerClampNearest, nullptr);
	//vkDestroySampler(context.device, s_SamplerRepeatNearest, nullptr);
}

void VulkanRendererBase::create_attachments()
{
	VkCommandBuffer cmd_buffer = begin_temp_cmd_buffer();

	/* Write Attachments */
	/* Create G-Buffers for Albedo, Normal, Depth for each Frame */
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		std::string s_prefix = "Frame #" + std::to_string(i) + "G-Buffer ";

		m_gbuffer_albedo[i].init(tex_base_color_format, render_width, render_height, 1, false);	/* G-Buffer Color */
		m_gbuffer_normal[i].init(tex_gbuffer_normal_format, render_width, render_height, 1, false);	/* G-Buffer Normal */
		m_gbuffer_depth[i].init(tex_gbuffer_depth_format, render_width, render_height, 1, false);		/* G-Buffer Depth */
		m_gbuffer_directional_shadow[i].init(tex_gbuffer_depth_format, render_width, render_height, 1, false);			/* G-Buffer Shadow map */
		m_gbuffer_metallic_roughness[i].init(tex_metallic_roughness_format, render_width, render_height, 1, false);			/* G-Buffer Metallic/Roughness */
		m_deferred_lighting_attachment[i].init(tex_deferred_lighting_format, 2048, 2048, 1, false); /* Final color attachment */

		m_gbuffer_albedo[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Albedo").c_str());
		m_gbuffer_normal[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Normal").c_str());
		m_gbuffer_depth[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Depth").c_str());
		m_gbuffer_directional_shadow[i].create(context.device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Directional Shadow Map").c_str());
		m_gbuffer_metallic_roughness[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, (s_prefix + "Metallic roughness").c_str());
		m_deferred_lighting_attachment[i].create(context.device, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Deferred Lighting Attachment");

		m_gbuffer_albedo[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_gbuffer_normal[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_gbuffer_depth[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_gbuffer_directional_shadow[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT });
		m_gbuffer_metallic_roughness[i].create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		m_deferred_lighting_attachment[i].create_view(context.device, {});

		m_gbuffer_albedo[i].transition(cmd_buffer, 					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		m_gbuffer_normal[i].transition(cmd_buffer, 					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		m_gbuffer_depth[i].transition(cmd_buffer, 					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		m_gbuffer_directional_shadow[i].transition(cmd_buffer, 		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		m_gbuffer_metallic_roughness[i].transition(cmd_buffer, 		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
		m_deferred_lighting_attachment[i].transition(cmd_buffer, 	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	}

	end_temp_cmd_buffer(cmd_buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderObjectManager::init()
{
	/* Mesh descriptor set layout */
	std::vector<VkDescriptorSetLayoutBinding> mesh_descriptor_set_bindings = {};
	mesh_descriptor_set_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
	mesh_descriptor_set_bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
	RenderObjectManager::mesh_descriptor_set_layout = create_descriptor_set_layout(mesh_descriptor_set_bindings);

	/* Drawable descriptor set layout */
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};
	bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT });  /* Object data (transforms etc) */
	drawable_descriptor_set_layout = create_descriptor_set_layout(bindings);

	/* Descriptor pool */
	const uint32_t max_number_meshes = 2048;
	const uint32_t max_number_drawables = 2048;
	uint32_t num_ssbo_per_mesh = 2;
	uint32_t num_mesh_descriptor_sets = max_number_meshes;
	uint32_t num_drawable_data_descriptor_sets = NUM_FRAMES * max_number_drawables;

	std::vector<VkDescriptorPoolSize> pool_sizes;
	pool_sizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = num_mesh_descriptor_sets * num_ssbo_per_mesh });
	pool_sizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = num_drawable_data_descriptor_sets * max_number_drawables });

	descriptor_pool = create_descriptor_pool(pool_sizes, num_mesh_descriptor_sets + num_drawable_data_descriptor_sets);

	/* Default material */
	uint32_t placeholder_tex_id = (uint32_t)add_texture("../../../data/textures/checker_grey.png", "Placeholder Texture");
	static Material default_material
	{
		.tex_base_color_id = placeholder_tex_id,
		.tex_metallic_roughness_id = 0,
		.tex_normal_id = 0,
		.tex_emissive_id = 0,
		.base_color_factor = glm::vec4(1.0f, 1.0, 1.0f, 1.0f),
		.metallic_factor = 0.0f,
		.roughness_factor = 0.0f,
	};
	add_material(default_material, "Default Material");
}

void RenderObjectManager::destroy()
{
	vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(context.device, drawable_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, mesh_descriptor_set_layout, nullptr);

	for (VulkanMesh& mesh : meshes)
	{
		mesh.destroy();
	}

	for (Texture& tex : textures)
	{
		tex.destroy();
	}
}


void RenderObjectManager::configure() {
	/* Drawables */
	/* Setup buffers */
	create_buffers();

	VkDescriptorBufferInfo drawable_ubo_desc_info = { .buffer = RenderObjectManager::object_data_dynamic_ubo, .offset = 0, .range = per_object_data_dynamic_aligment };
	VkDescriptorBufferInfo material_ubo_desc_info = { .buffer = material_data_ubo, .offset = 0, .range = VK_WHOLE_SIZE };
	drawable_descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::drawable_descriptor_set_layout);
	std::array<VkWriteDescriptorSet, 1> desc_writes
	{
		BufferWriteDescriptorSet(RenderObjectManager::drawable_descriptor_set, 0, &drawable_ubo_desc_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC),	/* Object data */
	};
	vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);

	/* Meshes */
	/* Setup descriptor set layout */

	/* Setup descriptor set */
	for (VulkanMesh& mesh : meshes)
	{
		VkDescriptorBufferInfo sbo_vtx_info = { .buffer = mesh.m_vertex_index_buffer, .offset = 0, .range = mesh.m_vertex_buf_size_bytes };
		VkDescriptorBufferInfo sbo_idx_info = { .buffer = mesh.m_vertex_index_buffer, .offset = mesh.m_vertex_buf_size_bytes, .range = mesh.m_index_buf_size_bytes };
		assert(mesh.descriptor_set);
		std::vector<VkWriteDescriptorSet> write_desc = {};
		write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 0, &sbo_vtx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 1, &sbo_idx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		vkUpdateDescriptorSets(context.device, (uint32_t)write_desc.size(), write_desc.data(), 0, nullptr);
	}
}

void RenderObjectManager::create_buffers()
{
	per_object_data_dynamic_aligment = (uint32_t)calc_dynamic_ubo_alignment(sizeof(TransformDataUbo));
	object_data_dynamic_ubo_size_bytes = (uint32_t)drawables.size() * per_object_data_dynamic_aligment;
	RenderObjectManager::object_data_dynamic_ubo.init(Buffer::Type::UNIFORM, (uint32_t)object_data_dynamic_ubo_size_bytes);

	per_object_datas = (TransformDataUbo*)alignedAlloc(object_data_dynamic_ubo_size_bytes, per_object_data_dynamic_aligment);

	/* Material info */
	material_data_ubo.init(Buffer::Type::UNIFORM, sizeof(Material));
}

void RenderObjectManager::add_drawable(std::string_view mesh_name, std::string_view name, DrawFlag flags,
                                       glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	add_drawable(get_mesh_id(mesh_name.data()), name.data(), flags, position, rotation, scale);
}
void RenderObjectManager::add_drawable(size_t mesh_id, const std::string& name, DrawFlag flags,
                                       glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	Drawable drawable;
	drawable.mesh_id = mesh_id;
	drawable.flags = flags;
	drawable.position = position;
	drawable.rotation = rotation;
	drawable.scale = scale;
	drawable.update_model_matrix();
	if (!drawable.has_primitives())
	{
		drawable.material_id = RenderObjectManager::get_material("Default Material").first;
	}
	drawable.id = drawables.size();
	size_t transform_data_idx = transform_datas.size();

	drawables.push_back(drawable);

	transform_datas.push_back(drawable.transform);
	drawable_from_name.insert({ name, drawable.id });
	drawable_datas_from_name.insert({ name, transform_data_idx });
	drawable_names.push_back(name);
}

size_t RenderObjectManager::add_texture(const std::string& filename, const std::string& name, VkFormat format, bool calc_mip)
{
	//std::string name = filename.substr(filename.find_last_of('/') + 1);
	Image im = load_image_from_file(filename);
	Texture2D tex2D;
	tex2D.init(format, im.w, im.h, 1, calc_mip);
	tex2D.create_from_data(&im, name);
	tex2D.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, tex2D.info.mipLevels });

	return add_texture(tex2D, name);
}

size_t RenderObjectManager::add_texture(const Texture2D& texture, const std::string& name)
{
	auto it = texture_from_name.find(name);
	if (it != texture_from_name.end())
	{
		return it->second;
	}
	size_t texture_idx = textures.size();
	textures.push_back(texture);
	texture_names.push_back(name);
	texture_from_name.insert({ name, texture_idx });
	return texture_idx;
}

size_t RenderObjectManager::add_material(Material material, const std::string& name)
{
	size_t material_idx = materials.size();
	material_names.push_back(name);
	materials.push_back(material);
	material_from_name.insert({ name, material_idx });
	return material_idx;
}

size_t RenderObjectManager::add_mesh(VulkanMesh& mesh, const std::string& name)
{
	mesh.descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::mesh_descriptor_set_layout);
	size_t mesh_idx = meshes.size();
	mesh_names.push_back(name);
	meshes.push_back(mesh);
	mesh_id_from_name.insert({ name, mesh_idx });
	return mesh_idx;
}

Drawable* RenderObjectManager::get_drawable(const std::string& name)
{
	auto ite = drawable_from_name.find(name);
	if (ite != drawable_from_name.end())
	{
		return &drawables[ite->second];
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

std::pair<size_t, Material*> RenderObjectManager::get_material(const std::string& name)
{
	auto ite = material_from_name.find(name);
	if (ite != material_from_name.end())
	{
		return { ite->second, &materials[ite->second] };
	}
	return { 0, nullptr };
}

std::pair<size_t, TransformData*> RenderObjectManager::get_drawable_data(const std::string& name)
{
	auto ite = drawable_datas_from_name.find(name);
	if (ite != drawable_datas_from_name.end())
	{
		return { ite->second, &transform_datas[ite->second] };
	}
	return {};
}

size_t RenderObjectManager::get_mesh_id(const std::string& name)
{
	auto ite = mesh_id_from_name.find(name);
	if (ite != mesh_id_from_name.end())
	{
		return ite->second;
	}
	return 0;
}

VulkanMesh* RenderObjectManager::get_mesh(const std::string& name)
{
	auto ite = mesh_id_from_name.find(name);
	if (ite != mesh_id_from_name.end())
	{
		return &meshes[ite->second];
	}
	return nullptr;
}

std::pair<size_t, Texture2D*> RenderObjectManager::get_texture(const std::string& name)
{
	auto ite = texture_from_name.find(name);
	if (ite != texture_from_name.end())
	{
		return { ite->second, &textures[ite->second] };
	}
	return { 0, nullptr };
}

void RenderObjectManager::update_per_object_data(const VulkanRendererBase::PerFrameData& frame_data)
{
	for (size_t i = 0; i < drawables.size(); i++)
	{
		const VulkanMesh& mesh = drawables[i].get_mesh();
		per_object_datas[i].model = transform_datas[i].model;
		per_object_datas[i].mvp = frame_data.proj * frame_data.view * transform_datas[i].model;
		per_object_datas[i].bbox_min_WS = per_object_datas[i].model * mesh.geometry_data.bbox_min_WS;
		per_object_datas[i].bbox_max_WS = per_object_datas[i].model * mesh.geometry_data.bbox_max_WS;
	}

	object_data_dynamic_ubo.upload(context.device, per_object_datas, 0, RenderObjectManager::object_data_dynamic_ubo_size_bytes);
}