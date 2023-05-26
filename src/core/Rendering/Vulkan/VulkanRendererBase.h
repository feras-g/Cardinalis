#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <Rendering/Vulkan/VulkanRenderInterface.h>
#include <Rendering/Vulkan/RenderPass.h>
#include <Rendering/Vulkan/VulkanMesh.h>
#include "DescriptorSet.h"

class VulkanRendererBase
{
public:
	struct PerFrameData
	{
		glm::mat4 view{};
		glm::mat4 proj{};
		glm::mat4 inv_view_proj{};
		glm::vec4 view_pos{};
	};
	
	static inline DescriptorSetLayout m_framedata_desc_set_layout;
	static inline DescriptorSet m_framedata_desc_set[NUM_FRAMES];

	static void create_descriptor_sets();
	static void create_samplers();
	static void create_buffers();
	static void update_frame_data(const PerFrameData& data, size_t current_frame_idx);
	static void destroy();


	static VkSampler s_SamplerRepeatLinear;
	static VkSampler s_SamplerClampLinear;
	static VkSampler s_SamplerClampNearest;
	static VkSampler s_SamplerRepeatNearest;
	static Buffer m_ubo_framedata[NUM_FRAMES];

	/* WIP */
	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
	VkPipeline m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorSet m_descriptor_set;


	static void create_attachments();

	static uint32_t render_width;
	static uint32_t render_height;

	/* G-Buffers for Deferred rendering */
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_albdedo;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_normal;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_depth;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_directional_shadow;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_metallic_roughness;
	static inline std::array <Texture2D, NUM_FRAMES> m_output_attachment;

	/* Formats */
	static const VkFormat color_attachment_format = VK_FORMAT_R8G8B8A8_SRGB; /* Format of the final render attachment */

	static inline std::vector<VkFormat> m_formats =
	{
		tex_base_color_format,				/* Base color / Albedo */
		tex_normal_format,		/* Vertex normal */
		tex_metallic_roughness_format,		/* Metallic roughness */
	};
	static inline VkFormat m_depth_format = VK_FORMAT_D16_UNORM;

	// void init();
	// void render();
	// void draw_scene();
	// void create_buffers()
	// void update_buffers(void* uniform_data, size_t data_size);
	// void update_descriptor_set(VkDevice device);
	// void create_attachments();
};

/**
 * @param ubo_element_size_bytes Size of a single uniform element contained in the dynamic uniform buffer
 */
static size_t calc_dynamic_ubo_alignment(size_t ubo_element_size_bytes)
{
	size_t min_ubo_alignment = VulkanRenderInterface::device_limits.minUniformBufferOffsetAlignment;
	size_t dynamic_aligment = ubo_element_size_bytes;
	if (min_ubo_alignment > 0) 
	{
		dynamic_aligment = (dynamic_aligment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
	}
	return dynamic_aligment;
}

static void* alignedAlloc(size_t size, size_t alignment)
{
	void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

class RenderObjectManager
{
public:
	RenderObjectManager() = delete;
	RenderObjectManager(const RenderObjectManager&) = delete;
	RenderObjectManager& operator=(const RenderObjectManager&) = delete;
	RenderObjectManager(RenderObjectManager&&) = delete;
public:

	/* Call after adding all drawables */
	static void configure()
	{
		/* Setup descriptor pool */
		uint32_t num_ssbo_per_mesh = 2;
		uint32_t num_mesh_descriptor_sets = (uint32_t)meshes.size();
		uint32_t num_drawable_data_descriptor_sets = NUM_FRAMES;

		std::vector<VkDescriptorPoolSize> pool_sizes;
		{
			pool_sizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = num_mesh_descriptor_sets * num_ssbo_per_mesh });
		}
		{
			pool_sizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = num_drawable_data_descriptor_sets * (uint32_t)drawables.size() });
		}

		descriptor_pool = create_descriptor_pool(pool_sizes, num_mesh_descriptor_sets + num_drawable_data_descriptor_sets);

		/* Drawables */
		/* Setup buffers */
		per_object_data_dynamic_aligment = (uint32_t)calc_dynamic_ubo_alignment(sizeof(TransformDataUbo));
		object_data_dynamic_ubo_size_bytes = (uint32_t)drawables.size() * per_object_data_dynamic_aligment;
		create_uniform_buffer(RenderObjectManager::object_data_dynamic_ubo, object_data_dynamic_ubo_size_bytes);

		per_object_datas   = (TransformDataUbo*)alignedAlloc(object_data_dynamic_ubo_size_bytes, per_object_data_dynamic_aligment);

		/* Setup descriptor set */
		std::vector<VkDescriptorSetLayoutBinding> bindings = {};
		bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT });  /* Object data (transforms etc) */

		drawable_descriptor_set_layout = create_descriptor_set_layout(bindings);
		drawable_descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::drawable_descriptor_set_layout);

		VkDescriptorBufferInfo object_ubo_desc_info = { .buffer = RenderObjectManager::object_data_dynamic_ubo.buffer, .offset = 0, .range = per_object_data_dynamic_aligment };

		/* Material info */
		create_uniform_buffer(material_data_ubo, sizeof(Material));
		VkDescriptorBufferInfo desc_ubo_material = {};
		desc_ubo_material.buffer = material_data_ubo.buffer;
		desc_ubo_material.offset = 0;
		desc_ubo_material.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo material_ubo_desc_info = { .buffer = material_data_ubo.buffer, .offset = 0, .range = VK_WHOLE_SIZE };
		std::array<VkWriteDescriptorSet, 1> desc_writes
		{
			BufferWriteDescriptorSet(RenderObjectManager::drawable_descriptor_set, 0, &object_ubo_desc_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC),	/* Object data */
		};
		vkUpdateDescriptorSets(context.device, (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);

		/* Meshes */
		/* Setup descriptor set layout */
		std::vector<VkDescriptorSetLayoutBinding> mesh_descriptor_set_bindings = {};
		mesh_descriptor_set_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		mesh_descriptor_set_bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		RenderObjectManager::mesh_descriptor_set_layout = create_descriptor_set_layout(mesh_descriptor_set_bindings);
		/* Setup descriptor set */
		for (VulkanMesh& mesh : meshes)
		{
			VkDescriptorBufferInfo sbo_vtx_info = { .buffer = mesh.m_vertex_index_buffer.buffer, .offset = 0, .range = mesh.m_vertex_buf_size_bytes };
			VkDescriptorBufferInfo sbo_idx_info = { .buffer = mesh.m_vertex_index_buffer.buffer, .offset = mesh.m_vertex_buf_size_bytes, .range = mesh.m_index_buf_size_bytes };
			mesh.descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::mesh_descriptor_set_layout);
			std::vector<VkWriteDescriptorSet> write_desc = {};
			write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 0, &sbo_vtx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
			write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 1, &sbo_idx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
			vkUpdateDescriptorSets(context.device, (uint32_t)write_desc.size(), write_desc.data(), 0, nullptr);
		}
	}

	static inline void add_drawable(Drawable drawable, const std::string& name, TransformData transform)
	{ 
		assert(drawable.mesh_handle);
		if (!drawable.has_primitives)
		{
			drawable.material_id = (uint32_t)RenderObjectManager::get_material("Default Material").first;
		}
		drawable.id = (uint32_t)drawables.size();
		size_t transform_data_idx = transform_datas.size();
		drawable.transform = transform;
		drawables.push_back(drawable);

		transform.model = drawable.mesh_handle->model * transform.model;
		/* Update scene bounding box */
		{
			glm::vec3 drawable_bbox_min_WS = drawable.mesh_handle->geometry_data.bbox_min_WS * transform.model;
			glm::vec3 drawable_bbox_max_WS = drawable.mesh_handle->geometry_data.bbox_max_WS * transform.model;

			global_bbox_min_WS.x = std::min(global_bbox_min_WS.x, drawable_bbox_min_WS.x);
			global_bbox_min_WS.y = std::min(global_bbox_min_WS.y, drawable_bbox_min_WS.y);
			global_bbox_min_WS.z = std::min(global_bbox_min_WS.z, drawable_bbox_min_WS.z);

			global_bbox_max_WS.x = std::max(global_bbox_max_WS.x, drawable_bbox_max_WS.x);
			global_bbox_max_WS.y = std::max(global_bbox_max_WS.y, drawable_bbox_max_WS.y);
			global_bbox_max_WS.z = std::max(global_bbox_max_WS.z, drawable_bbox_max_WS.z);

			LOG_ERROR("Updated scene bbox : min ({},{},{}),  max ({},{},{}))", global_bbox_min_WS.x, global_bbox_min_WS.y, global_bbox_min_WS.z, global_bbox_max_WS.x, global_bbox_max_WS.y, global_bbox_max_WS.z);
		}


		transform_datas.push_back(transform);
		drawable_from_name.insert({ name, drawable.id });
		drawable_datas_from_name.insert({ name, transform_data_idx });
		drawable_names.push_back(name);
	}

	static inline size_t add_texture(const std::string& filename, const std::string& name, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool calc_mip = true)
	{
		//std::string name = filename.substr(filename.find_last_of('/') + 1);
		Image im = load_image_from_file(filename);
		Texture2D tex2D;
		tex2D.init(format, im.w, im.h, 1, calc_mip);
		tex2D.create_from_data(&im, name);
		tex2D.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, tex2D.info.mipLevels });

		return add_texture(tex2D, name);
	}

	static inline size_t add_texture(const Texture2D& texture, const std::string& name)
	{
		auto it = texture_from_name.find(name);
		if (it != texture_from_name.end())
		{
			return it->second;
		}
		size_t texture_idx = textures.size();
		textures.push_back(texture);
		texture_names.push_back(name);
		texture_from_name.insert({name, texture_idx});
		return texture_idx;
	}

	static inline size_t add_material(Material material, const std::string& name)
	{ 
		size_t material_idx = materials.size();
		material_names.push_back(name);
		materials.push_back(material);
		material_from_name.insert({name, material_idx});
		return material_idx;
	}

	static inline void add_mesh(VulkanMesh& mesh, const std::string& name)
	{
		size_t mesh_idx = meshes.size();
		mesh_names.push_back(name);
		meshes.push_back(mesh);
		mesh_from_name.insert({ name, mesh_idx });
	}

	static inline void add_mesh(VulkanMesh&& mesh, const std::string&& name)
	{
		size_t mesh_idx = meshes.size();
		mesh_names.emplace_back(name);
		meshes.emplace_back(mesh);
		mesh_from_name.insert({ name, mesh_idx });
	}

	static inline Drawable* get_drawable(const std::string& name)
	{
		auto ite = drawable_from_name.find(name);
		if (ite != drawable_from_name.end())
		{
			return &drawables[ite->second];
		}
		return nullptr;
	}

	static std::pair<size_t, Material*> get_material(const std::string& name)
	{
		auto ite = material_from_name.find(name);
		if (ite != material_from_name.end())
		{
			return { ite->second, &materials[ite->second] };
		}
		return {0, nullptr};
	}

	static std::pair<size_t, TransformData*> get_drawable_data(const std::string& name)
	{
		auto ite = drawable_datas_from_name.find(name);
		if (ite != drawable_datas_from_name.end())
		{
			return { ite->second, &transform_datas[ite->second] };
		}
		return {};
	}

	static VulkanMesh* get_mesh(const std::string& name)
	{
		auto ite = mesh_from_name.find(name);
		if (ite != mesh_from_name.end())
		{
			return &meshes[ite->second];
		}
		return nullptr;
	}

	static std::pair<size_t, Texture2D*> get_texture(const std::string& name)
	{
		auto ite = texture_from_name.find(name);
		if (ite != texture_from_name.end())
		{
			return { ite->second, &textures[ite->second] };
		}
		return { 0, nullptr };
	}

	static void update_per_object_data(const VulkanRendererBase::PerFrameData& frame_data)
	{
		for (size_t i = 0; i < drawables.size(); i++)
		{
			per_object_datas[i].model = transform_datas[i].model;
			per_object_datas[i].mvp = frame_data.proj *frame_data.view * transform_datas[i].model ;
			per_object_datas[i].bbox_min_WS = per_object_datas[i].model * drawables[i].mesh_handle->geometry_data.bbox_min_WS;
			per_object_datas[i].bbox_max_WS = per_object_datas[i].model * drawables[i].mesh_handle->geometry_data.bbox_max_WS ;
		}

		upload_buffer_data(object_data_dynamic_ubo, per_object_datas, RenderObjectManager::object_data_dynamic_ubo_size_bytes, 0);
	}

	static void destroy()
	{
		vkDeviceWaitIdle(context.device);
		vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(context.device, drawable_descriptor_set_layout, nullptr);
		vkDestroyDescriptorSetLayout(context.device, mesh_descriptor_set_layout, nullptr);
	}

	static inline std::vector<VulkanMesh> meshes;
	static inline std::vector<Drawable> drawables;
	static inline std::vector<Material> materials;
	static inline std::vector<Texture2D> textures;

	static inline std::unordered_map<std::string, size_t> mesh_from_name;
	static inline std::unordered_map<std::string, size_t> drawable_from_name;
	static inline std::unordered_map<std::string, size_t> drawable_datas_from_name;
	static inline std::unordered_map<std::string, size_t> material_from_name;
	static inline std::unordered_map<std::string, size_t> texture_from_name;

	static inline std::vector<std::string> mesh_names;
	static inline std::vector<std::string> drawable_names;
	static inline std::vector<std::string> material_names;
	static inline std::vector<std::string> texture_names;

	static inline std::vector<TransformData> transform_datas;
	static inline TransformDataUbo* per_object_datas;


	static inline VkDescriptorPool descriptor_pool;

	static inline VkDescriptorSet drawable_descriptor_set;
	static inline VkDescriptorSetLayout drawable_descriptor_set_layout;
	static inline VkDescriptorSetLayout mesh_descriptor_set_layout;

	/* Uniform array containing object data for each drawable */
	static inline Buffer object_data_dynamic_ubo;

	/* Uniform buffer containing material data for a drawable */
	static inline Buffer material_data_ubo;

	static inline uint32_t per_object_data_dynamic_aligment = 0;
	static inline uint32_t object_data_dynamic_ubo_size_bytes = 0;

	/* Bounding-box encompassing all the rendered drawables */
	static inline glm::vec3 global_bbox_min_WS { FLT_MAX };
	static inline glm::vec3 global_bbox_max_WS { FLT_MIN };
private:
};


