#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <core/rendering/vulkan/VulkanRenderInterface.h>
#include <core/rendering/vulkan/RenderPass.h>
#include <core/rendering/vulkan/VulkanMesh.h>
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

	static VulkanRendererBase& get_instance() { static VulkanRendererBase rb; return rb; };

	static inline DescriptorSetLayout m_framedata_desc_set_layout;
	static inline DescriptorSet m_framedata_desc_set[NUM_FRAMES];

	void create_descriptor_sets();
	void create_samplers();
	void create_buffers();
	void update_frame_data(const PerFrameData& data, size_t current_frame_idx);
	
	~VulkanRendererBase();

	static VkSampler s_SamplerRepeatLinear;
	static VkSampler s_SamplerClampLinear;
	static VkSampler s_SamplerClampNearest;
	static VkSampler s_SamplerRepeatNearest;
	Buffer m_ubo_framedata[NUM_FRAMES];

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
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_albedo;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_normal;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_depth;
	static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_metallic_roughness;
	//static inline std::array<Texture2D, NUM_FRAMES> m_gbuffer_emissive;

	/* Color attachment resulting the deferred lighting pass */
	static inline std::array <Texture2D, NUM_FRAMES> m_deferred_lighting_attachment;

	/* Formats */
	static VkFormat swapchain_color_format;
	static VkColorSpaceKHR swapchain_colorspace;
	static VkFormat swapchain_depth_format;
	static VkFormat tex_base_color_format;
	static VkFormat tex_metallic_roughness_format;
	static VkFormat tex_normal_map_format;
	static VkFormat tex_emissive_format;
	static VkFormat tex_gbuffer_normal_format;
	static VkFormat tex_gbuffer_depth_format;
	static VkFormat tex_deferred_lighting_format;


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

struct RenderObjectManager
{
	void init();
	void destroy();
	/* Call after adding all drawables */
	void configure();
	void create_buffers();

	static void add_drawable(size_t mesh_id, const std::string& name, DrawFlag flags = DrawFlag::VISIBLE | DrawFlag::CAST_SHADOW,
	                         glm::vec3 position = {0,0,0}, glm::vec3 rotation = {0,0,0}, glm::vec3 scale = {1,1,1});
	static void add_drawable(std::string_view mesh_name, std::string_view name, DrawFlag flags,
	                         glm::vec3 position = {0,0,0}, glm::vec3 rotation = {0,0,0}, glm::vec3 scale = {1,1,1});
	static size_t add_texture(const std::string& filename, const std::string& name, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool calc_mip = true);
	static size_t add_texture(const Texture2D& texture, const std::string& name);
	static size_t add_material(Material material, const std::string& name);
	static size_t add_mesh(VulkanMesh& mesh, const std::string& name);
	static Drawable* get_drawable(const std::string& name);
	static std::pair<size_t, Material*> get_material(const std::string& name);
	static std::pair<size_t, TransformData*> get_drawable_data(const std::string& name);
	static VulkanMesh* get_mesh(const std::string& name);
	static size_t get_mesh_id(const std::string& name);
	static std::pair<size_t, Texture2D*> get_texture(const std::string& name);
	static void update_per_object_data(const VulkanRendererBase::PerFrameData& frame_data);

	static inline std::vector<VulkanMesh> meshes;
	static inline std::vector<Drawable> drawables;
	static inline std::vector<Material> materials;
	static inline std::vector<Texture2D> textures;

	static inline std::unordered_map<std::string, size_t> mesh_id_from_name;
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

