#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <Rendering/Vulkan/VulkanRenderInterface.h>
#include <Rendering/Vulkan/RenderPass.h>
#include <Rendering/Vulkan/VulkanMesh.h>

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

	static void create_samplers();
	static void create_buffers();
	static void update_frame_data(const PerFrameData& data);
	static void destroy();

	static VkSampler s_SamplerRepeatLinear;
	static VkSampler s_SamplerClampLinear;
	static VkSampler s_SamplerClampNearest;
	static Buffer m_ubo_common_framedata;

	/* WIP */
	vk::DynamicRenderPass m_dyn_renderpass[NUM_FRAMES];
	VkPipeline m_gfx_pipeline = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool;
	VkPipelineLayout m_pipeline_layout;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorSet m_descriptor_set;

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
		uint32_t num_mesh_descriptor_sets = meshes.size();
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
		drawable_data_dynamic_aligment = calc_dynamic_ubo_alignment(2 * sizeof(glm::mat4));
		object_data_dynamic_ubo_size_bytes = drawables.size() * drawable_data_dynamic_aligment;
		create_uniform_buffer(RenderObjectManager::object_data_dynamic_ubo, object_data_dynamic_ubo_size_bytes);

		drawable_ubo_datas   = (TransformDataUbo*)alignedAlloc(object_data_dynamic_ubo_size_bytes, drawable_data_dynamic_aligment);

		/* Setup descriptor set layout */
		std::vector<VkDescriptorSetLayoutBinding> bindings = {};
		bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT });  /* Object data (transforms etc) */
		drawable_descriptor_set_layout = create_descriptor_set_layout(bindings);

		/* Setup descriptor set */
		drawable_descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::drawable_descriptor_set_layout);
		VkDescriptorBufferInfo object_ubo_desc_info = { .buffer = RenderObjectManager::object_data_dynamic_ubo.buffer, .offset = 0, .range = drawable_data_dynamic_aligment };
		std::array<VkWriteDescriptorSet, 1> desc_writes
		{
			BufferWriteDescriptorSet(RenderObjectManager::drawable_descriptor_set, 0, &object_ubo_desc_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		};
		vkUpdateDescriptorSets(context.device, desc_writes.size(), desc_writes.data(), 0, nullptr);

		/* Meshes */
		/* Setup descriptor set layout */
		std::vector<VkDescriptorSetLayoutBinding> mesh_descriptor_set_bindings = {};
		mesh_descriptor_set_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		mesh_descriptor_set_bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		RenderObjectManager::mesh_descriptor_set_layout = create_descriptor_set_layout(mesh_descriptor_set_bindings);
		/* Setup descriptor set */
		for (VulkanMesh& mesh : meshes)
		{
			VkDescriptorBufferInfo sbo_vtx_info = { .buffer = mesh.m_vertex_index_buffer.buffer, .offset = 0, .range = mesh.m_VtxBufferSizeInBytes };
			VkDescriptorBufferInfo sbo_idx_info = { .buffer = mesh.m_vertex_index_buffer.buffer, .offset = mesh.m_VtxBufferSizeInBytes, .range = mesh.m_IdxBufferSizeInBytes };
			mesh.descriptor_set = create_descriptor_set(RenderObjectManager::descriptor_pool, RenderObjectManager::mesh_descriptor_set_layout);
			std::vector<VkWriteDescriptorSet> write_desc = {};
			write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 0, &sbo_vtx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
			write_desc.push_back(BufferWriteDescriptorSet(mesh.descriptor_set, 1, &sbo_idx_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
			vkUpdateDescriptorSets(context.device, write_desc.size(), write_desc.data(), 0, nullptr);
		}

	}

	static inline void add_drawable(const Drawable& drawable, const std::string& name, const TransformData& transform)
	{ 
		size_t drawable_idx = drawables.size();
		size_t transform_data_idx = transform_datas.size();
		drawables.push_back(drawable);
		transform_datas.push_back(transform);
		drawable_from_name.insert({ name, drawable_idx });
		drawable_datas_from_name.insert({ name, transform_data_idx });
		drawable_names.push_back(name);
	}


	static inline void add_material(const Material material, const std::string& name)
	{ 
		size_t material_idx = materials.size();
		material_names.push_back(name);
		materials.push_back(material);
		material_from_name.insert({name, material_idx});
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

	static Material* get_material(const std::string& name)
	{
		auto ite = material_from_name.find(name);
		if (ite != material_from_name.end())
		{
			return &materials[ite->second];
		}
		return nullptr;
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

	static void update_drawables(const VulkanRendererBase::PerFrameData& frame_data)
	{
		for (size_t i = 0; i < drawables.size(); i++)
		{
			glm::mat4* mvp_ptr   = &drawable_ubo_datas[i].mvp;
			glm::mat4* model_ptr = &drawable_ubo_datas[i].model;

			glm::mat4 model = glm::identity<glm::mat4>(); 

			model = glm::translate(model, glm::vec3(transform_datas[i].translation)); 
			model = glm::rotate(model, transform_datas[i].rotation.w, glm::vec3(transform_datas[i].rotation));
			model = glm::scale(model, glm::vec3(transform_datas[i].scale));
			
			*mvp_ptr = frame_data.proj * frame_data.view * model;
			*model_ptr = model;
		}

		upload_buffer_data(object_data_dynamic_ubo, drawable_ubo_datas, RenderObjectManager::object_data_dynamic_ubo_size_bytes, 0);
	}

	static void destroy()
	{
		vkDeviceWaitIdle(context.device);
		vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(context.device, drawable_descriptor_set_layout, nullptr);
		vkDestroyDescriptorSetLayout(context.device, mesh_descriptor_set_layout, nullptr);
	}

	static inline std::vector<Material> materials;

	static inline std::unordered_map<std::string, size_t> material_from_name;
	static inline std::unordered_map<std::string, size_t> mesh_from_name;
	static inline std::unordered_map<std::string, size_t> drawable_from_name;
	static inline std::unordered_map<std::string, size_t> drawable_datas_from_name;

	static inline std::vector<std::string> material_names;

	static inline std::vector<Drawable> drawables;
	static inline std::vector<std::string> drawable_names;
	static inline std::vector<TransformData> transform_datas;
	static inline TransformDataUbo* drawable_ubo_datas;

	static inline std::vector<VulkanMesh> meshes;
	static inline std::vector<std::string> mesh_names;



	static inline VkDescriptorPool descriptor_pool;

	static inline VkDescriptorSet drawable_descriptor_set;
	static inline VkDescriptorSetLayout drawable_descriptor_set_layout;
	static inline VkDescriptorSetLayout mesh_descriptor_set_layout;

	/* Uniform array containing object data for each drawable */
	static inline Buffer object_data_dynamic_ubo;

	static inline uint32_t drawable_data_dynamic_aligment = 0;
	static inline uint32_t object_data_dynamic_ubo_size_bytes = 0;
private:
};


