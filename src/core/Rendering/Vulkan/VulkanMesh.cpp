#include "VulkanMesh.h"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/engine/Image.h"
#include "RenderObjectManager.h"

#include "glm/gtx/euler_angles.hpp"

#include <vector>
#include <span>
#include <unordered_set>

//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//#include <assimp/cimport.h>
//#include <assimp/version.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static std::string base_path;

void VulkanMesh::create_from_file(const std::string& filename)
{
	std::string_view ext = get_extension(filename);

	std::string models_path = "../../../data/models/";
	
	if (ext == "glb" || ext == "gltf")
	{
		create_from_file_gltf(models_path + filename);
	}
	else
	{
		assert(false);
	}
}

void VulkanMesh::create_from_data(std::span<VertexData> vertices, std::span<unsigned int> indices)
{
	m_num_vertices = vertices.size();
	m_num_indices = indices.size();
	m_index_buf_size_bytes  = EngineUtils::round_to(indices.size() * sizeof(unsigned int), RenderInterface::device_limits.minStorageBufferOffsetAlignment);
	m_vertex_buf_size_bytes = EngineUtils::round_to(vertices.size() * sizeof(VertexData), RenderInterface::device_limits.minStorageBufferOffsetAlignment);
	
	create_vertex_index_buffer(m_vertex_index_buffer, vertices.data(), m_vertex_buf_size_bytes, indices.data(), m_index_buf_size_bytes);
}

void VulkanMesh::destroy()
{
	m_vertex_index_buffer.destroy();
}

static void load_vertices(Primitive p, cgltf_primitive* primitive, GeometryData& geometry)
{
	std::vector<glm::vec3> positionsBuffer;
	std::vector<glm::vec3> normalsBuffer;
	std::vector<glm::vec2> texCoordBuffer;
	std::vector<glm::vec3> tangentBuffer;

	// Build raw buffers containing each attributes components
	for (int attribIdx = 0; attribIdx < primitive->attributes_count; ++attribIdx)
	{
		cgltf_attribute* attribute = &primitive->attributes[attribIdx];

		switch (attribute->type)
		{
		case cgltf_attribute_type_position:
		{
			for (int i = 0; i < attribute->data->count; ++i)
			{
				glm::vec3 pos;
				cgltf_accessor_read_float(attribute->data, i, &pos.x, 3);
				positionsBuffer.push_back(pos);
			}

			// Also get bounding box for this primitive
			if (attribute->data->has_min)
			{
				glm::vec4 bbox_min_OS (attribute->data->min[0], attribute->data->min[1], attribute->data->min[2], 1.0f);
				geometry.bbox_min_WS = bbox_min_OS * geometry.world_mat;
			}

			if (attribute->data->has_max)
			{
				glm::vec4 bbox_max_OS(attribute->data->max[0], attribute->data->max[1], attribute->data->max[2], 1.0f);
				geometry.bbox_max_WS = bbox_max_OS * geometry.world_mat;
			}
		}
		break;

		case cgltf_attribute_type_normal:
		{
			glm::vec3 normal;
			for (int i = 0; i < attribute->data->count; ++i)
			{
				cgltf_accessor_read_float(attribute->data, i, &normal.x, 3);
				normalsBuffer.push_back(normal);
			}
		}
		break;

		case cgltf_attribute_type_texcoord:
		{
			for (int i = 0; i < attribute->data->count; i++)
			{
				glm::vec2 texcoord;
				cgltf_accessor_read_float(attribute->data, i, &texcoord.x, 2);
				texCoordBuffer.push_back(texcoord);
			}

		}
		break;

		case cgltf_attribute_type_tangent:
		{
			for (int i = 0; i < attribute->data->count; ++i)
			{
				glm::vec4 t;
				cgltf_accessor_read_float(attribute->data, i, &t.x, 4);
				tangentBuffer.push_back(glm::vec3(t.x, t.y, t.z) * t.w);
			}
		}
		break;
		}
	}

	// Build vertices
	for (int i = 0; i < positionsBuffer.size(); ++i)
	{
		VertexData vertex;
		vertex.pos = positionsBuffer[i];
		vertex.normal = normalsBuffer[i];
		if (texCoordBuffer.size())
		{
			vertex.uv = glm::vec3(texCoordBuffer[i], 0.0);
		}

		geometry.vertices.push_back(vertex);
	}
}

static void load_material(cgltf_primitive* gltf_primitive, Primitive& primitive)
{
	// https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/
	cgltf_material* gltf_mat = gltf_primitive->material;
	
	Material material;

	if (nullptr == gltf_mat)
	{
		primitive.material_id = ObjectManager::get_instance().default_material_id;
	}
	else
	{
		std::function load_tex = [](TextureType tex_type, cgltf_texture* tex, VkFormat format, bool calc_mip) -> int
		{
			const char* uri = tex->image->uri;
			std::string name = uri ? base_path + uri : base_path + tex->image->name;

			const ObjectManager& object_manager = ObjectManager::get_instance();

			/* Load from file path */
			int texture_id = ObjectManager::get_instance().get_texture_id(name.c_str());

			if (texture_id != -1)
			{
				return texture_id;
			}

			if (uri)
			{
				Image im = load_image_from_file(base_path + uri);
				Texture2D texture;
				texture.init(format, im.w, im.h, 1, false, name);
				texture.create_from_data(&im);
				texture.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, texture.info.mipLevels });

				if (tex_type == TextureType::NORMAL_MAP || tex_type == TextureType::METALLIC_ROUGHNESS_MAP)
				{
					texture.sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatNearest;
				}
				else
				{
					texture.sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatLinear;
				}
				texture.info.debugName = name.c_str();

				return ObjectManager::get_instance().add_texture(texture);
			}
			else
			{
				/* Load from buffer */
				assert(false);
				return 0;
			}
		};


		cgltf_texture* tex_normal = gltf_mat->normal_texture.texture;
		if (tex_normal)
		{
			material.texture_normal_map_idx = load_tex(TextureType::NORMAL_MAP, tex_normal, VulkanRendererCommon::get_instance().tex_normal_map_format, true);
		}

		cgltf_texture* tex_emissive = gltf_mat->emissive_texture.texture;
		if (tex_emissive)
		{
			material.texture_emissive_map_idx = load_tex(TextureType::EMISSIVE_MAP, tex_emissive, VulkanRendererCommon::get_instance().tex_emissive_format, true);
		}

		if (gltf_mat->has_pbr_metallic_roughness)
		{
			cgltf_texture* tex_base_color = gltf_mat->pbr_metallic_roughness.base_color_texture.texture;
			if (tex_base_color)
			{
				material.texture_base_color_idx = load_tex(TextureType::ALBEDO_MAP, tex_base_color, VulkanRendererCommon::get_instance().tex_base_color_format, true);
			}

			cgltf_texture* tex_metallic_roughness = gltf_mat->pbr_metallic_roughness.metallic_roughness_texture.texture;
			if (tex_metallic_roughness)
			{
				material.texture_metalness_roughness_idx = load_tex(TextureType::METALLIC_ROUGHNESS_MAP, tex_metallic_roughness, VulkanRendererCommon::get_instance().tex_metallic_roughness_format, false);
			}

			/* Factors */
			{
				//float* v = gltf_mat->pbr_metallic_roughness.base_color_factor;
				//material.base_color_factor = glm::vec4(v[0], v[1], v[2], v[3]);
				//material.metallic_factor = gltf_mat->pbr_metallic_roughness.metallic_factor;
				//material.roughness_factor = gltf_mat->pbr_metallic_roughness.roughness_factor;
			}
		}

		else if (gltf_mat->has_pbr_specular_glossiness)
		{
			assert(false);
		}

		primitive.material_id = ObjectManager::get_instance().add_material(material);
	}
}

static void load_primitive(cgltf_node* node, cgltf_primitive* primitive, Node* engine_node, GeometryData& geometry)
{
	Primitive p = {};
	p.first_index = (uint32_t)geometry.indices.size();
	p.index_count = (uint32_t)primitive->indices->count;
	
	glm::mat4 mesh_local_mat;
	cgltf_node_transform_world(node, glm::value_ptr(mesh_local_mat));
	p.model = mesh_local_mat;

	/* Load indices */
	for (uint32_t idx = 0; idx < p.index_count; idx++)
	{
		size_t index = cgltf_accessor_read_index(primitive->indices, idx);
		geometry.indices.push_back((unsigned int)(index + geometry.vertices.size()));
	}

	load_vertices(p, primitive, geometry);
	load_material(primitive, p);
	geometry.primitives.push_back(p);
}

static void process_node(cgltf_node* p_node, Node* parent, VulkanMesh& model)
{
	Node* node = new Node{};
	
	cgltf_node_transform_world(p_node, glm::value_ptr(node->matrix));
	node->parent = parent;

	if (p_node->mesh)
	{	
		for (int i = 0; i < p_node->mesh->primitives_count; ++i)
		{
			load_primitive(p_node, &p_node->mesh->primitives[i], node, model.geometry_data);
		}
	}

	if (p_node->light)
	{
		cgltf_light* light = p_node->light;
		
		if (light->type == cgltf_light_type_point)
		{
			model.geometry_data.punctual_lights_positions.push_back
			(
				glm::vec4(
					p_node->translation[0],
					p_node->translation[1],
					p_node->translation[2],
					1.0
				)
			);
			
			model.geometry_data.punctual_lights_colors.push_back
			(
				glm::vec4(
					light->color[0],
					light->color[1],
					light->color[2],
					1.0
				)
			);
		}
	}

	if (parent) 
	{
		parent->children.push_back(node);
	}
	else 
	{
		model.nodes.push_back(node);
	}

	//for (size_t i = 0; i < p_node->children_count; i++)
	//{
	//	process_node(p_node->children[i], node, model);
	//}
}


static void load_textures(cgltf_texture* textures, size_t texture_count)
{
	std::function load_tex = [](cgltf_texture* tex, VkFormat format, bool calc_mip) -> int
		{
			const char* uri = tex->image->uri;
			std::string name = uri ? base_path + uri : base_path + tex->image->name;

			const ObjectManager& object_manager = ObjectManager::get_instance();

			/* Load from file path */
			int texture_id = ObjectManager::get_instance().get_texture_id(name.c_str());

			if (texture_id != -1)
			{
				return texture_id;
			}

			if (uri)
			{
				Image im = load_image_from_file(base_path + uri);
				Texture2D texture;
				texture.init(format, im.w, im.h, 1, false, name);
				texture.create_from_data(&im);
				texture.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, texture.info.mipLevels });
				texture.sampler = VulkanRendererCommon::get_instance().s_SamplerRepeatLinear;
				texture.info.debugName = name.c_str();

				return ObjectManager::get_instance().add_texture(texture);
			}
			else
			{
				/* Load from buffer */
				assert(false);
				return 0;
			}
		};


	for (size_t i = 0; i < texture_count; i++)
	{
	}
}


void VulkanMesh::create_from_file_gltf(const std::string& filename)
{
	auto start = std::chrono::steady_clock::now();

	cgltf_options options = { };
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, filename.c_str(), &data);

	auto const pos = filename.find_last_of('/');
	
	base_path = filename.substr(0, pos + 1);
	
	if (result == cgltf_result_success)
	{
		result = cgltf_load_buffers(&options, data, filename.c_str());
		
		if (result == cgltf_result_success)
		{
			load_textures(data->textures, data->textures_count);

			for (size_t i = 0; i < data->nodes_count; ++i)
			{
				process_node(&data->nodes[i], nullptr, *this);
			}
		}

		m_num_vertices = geometry_data.vertices.size();
		m_num_indices  = geometry_data.indices.size();

		m_index_buf_size_bytes = m_num_indices * sizeof(unsigned int);
		m_vertex_buf_size_bytes = m_num_vertices * sizeof(VertexData);

		model = geometry_data.world_mat;

		create_from_data(geometry_data.vertices, geometry_data.indices);

		LOG_INFO("Loaded {} : {} Materials, {} Textures, Meshes : {}", filename, data->materials_count, data->textures_count, data->meshes_count);

		cgltf_free(data);
	}
	else
	{
		LOG_ERROR("Could not read GLTF/GLB file. [error code : {0}]", result);
		assert(false);
	}
	
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	LOG_WARN("Loaded GLTF model in {} s", elapsed_seconds.count());
}
