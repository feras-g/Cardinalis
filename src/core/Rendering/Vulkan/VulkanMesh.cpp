#include "VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "VulkanTools.h"

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
	
	if (ext == "glb" || ext == "gltf")
	{
		create_from_file_gltf(filename);
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
	m_index_buf_size_bytes = indices.size() * sizeof(unsigned int);
	m_vertex_buf_size_bytes = vertices.size() * sizeof(VertexData);

	create_vertex_index_buffer(m_vertex_index_buffer, vertices.data(), m_vertex_buf_size_bytes, indices.data(), m_index_buf_size_bytes);
}

void Drawable::draw(VkCommandBuffer cmd_buffer) const
{
	VulkanMesh& mesh = RenderObjectManager::meshes[mesh_id];
	vkCmdDraw(cmd_buffer, (uint32_t)mesh.m_num_indices, 1, 0, 0);
}

void Drawable::draw_node( VkCommandBuffer cmd_buffer, Node* node, VulkanMesh* model, VkPipelineLayout ppl_layout) 
{
	if (model->geometry_data.primitives.size() > 0) 
	{
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node->matrix;
		Node* currentParent = node->parent;
		while (currentParent) 
		{
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		nodeMatrix *= model->model;
		// Pass the final matrix to the vertex shader using push constants
		vkCmdPushConstants(cmd_buffer, ppl_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

		for (Primitive& primitive : model->geometry_data.primitives)
		{
			if (primitive.index_count > 0) 
			{
				vkCmdPushConstants(cmd_buffer, ppl_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), &RenderObjectManager::materials[primitive.material_id]);
				vkCmdDraw(cmd_buffer, primitive.index_count, 1, primitive.first_index, 0);
			}
		}
	}

	for (auto& child : node->children) 
	{
		draw_node(cmd_buffer, child, model, ppl_layout);
	}
}

void Drawable::draw_primitives(VkCommandBuffer cmd_buffer) const
{
	VulkanMesh& mesh = RenderObjectManager::meshes[mesh_id];
	for (int i = 0; i < mesh.geometry_data.primitives.size(); i++)
	{
		const Primitive& p = mesh.geometry_data.primitives[i];
		vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
	}
}

void Drawable::update_model_matrix()
{
	glm::mat4 T = glm::identity<glm::mat4>();
	T  = glm::translate(T, position);
	T *= glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z));
	T  = glm::scale(T, scale);
	transform.model = T;
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
		vertex.uv = glm::vec2(texCoordBuffer[i]);
		vertex.tangent = tangentBuffer.empty() ? glm::vec3(0) : tangentBuffer[i];

		geometry.vertices.push_back(vertex);
	}
}

static void load_material(cgltf_primitive* gltf_primitive, Primitive& primitive)
{
	// https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/
	cgltf_material* gltf_mat = gltf_primitive->material;
	
	Material material;
	if (!gltf_mat)
	{
		primitive.material_id = RenderObjectManager::get_material("Default Material").first;
	}
	else
	{
		/*
			Base Color
			Normal
			MetallicRoughness
			Emissive
		*/

		std::function load_tex = [](cgltf_texture* tex, VkFormat format, bool calc_mip) -> size_t
		{
			const char* uri = tex->image->uri;
			std::string name = uri ? base_path + uri : base_path + tex->image->name;

			std::pair<size_t, Texture2D*> tex_object = RenderObjectManager::get_texture(name);

			if (tex_object.second == nullptr)
			{
				if (uri)
				{
					/* Load from file path */
					return RenderObjectManager::add_texture(base_path + uri, name, format, calc_mip);
				}
				else
				{
					/* Load from buffer */
					assert(false);
					return 0;
				}
			}
			else
			{
				return tex_object.first;
			}
		};


		cgltf_texture* tex_normal = gltf_mat->normal_texture.texture;
		if (tex_normal)
		{
			material.tex_normal_id = (unsigned int)load_tex(tex_normal, tex_normal_format, true);
		}

		cgltf_texture* tex_emissive = gltf_mat->emissive_texture.texture;
		if (tex_emissive)
		{
			material.tex_emissive_id = (unsigned int)load_tex(tex_emissive, tex_emissive_format, true);
		}

		if (gltf_mat->has_pbr_metallic_roughness)
		{
			cgltf_texture* tex_base_color = gltf_mat->pbr_metallic_roughness.base_color_texture.texture;
			if (tex_base_color)
			{
				material.tex_base_color_id = (unsigned int)load_tex(tex_base_color, tex_base_color_format, true);
			}

			cgltf_texture* tex_metallic_roughness = gltf_mat->pbr_metallic_roughness.metallic_roughness_texture.texture;
			if (tex_metallic_roughness)
			{
				material.tex_metallic_roughness_id = (unsigned int)load_tex(tex_metallic_roughness, tex_metallic_roughness_format, false);
			}

			/* Factors */
			{
				float* v = gltf_mat->pbr_metallic_roughness.base_color_factor;
				material.base_color_factor = glm::vec4(v[0], v[1], v[2], v[3]);
				material.metallic_factor = gltf_mat->pbr_metallic_roughness.metallic_factor;
				material.roughness_factor = gltf_mat->pbr_metallic_roughness.roughness_factor;
			}
		}

		else if (gltf_mat->has_pbr_specular_glossiness)
		{
			assert(false);
		}

		std::size_t hash = MyHash<Material>{}(material);
		std::string material_name = std::to_string(hash);

		std::pair<size_t, Material*> mat_object = RenderObjectManager::get_material(material_name);
		if (mat_object.second == nullptr)
		{
			primitive.material_id = RenderObjectManager::add_material(material, material_name);
		}
		else
		{
			primitive.material_id = mat_object.first;
		}
	}
}

static void load_primitive(cgltf_node* node, cgltf_primitive* primitive, Node* engine_node, GeometryData& geometry)
{
	Primitive p = {};
	p.first_index = (uint32_t)geometry.indices.size();
	p.index_count = (uint32_t)primitive->indices->count;

	glm::mat4 mesh_local_mat;
	cgltf_node_transform_world(node, glm::value_ptr(mesh_local_mat));
	p.mat_model = mesh_local_mat;

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

static void process_node(cgltf_node* p_Node, Node* parent, VulkanMesh& model)
{
	Node* node = new Node{};
	
	cgltf_node_transform_world(p_Node, glm::value_ptr(node->matrix));
	node->parent = parent;

	if (p_Node->children_count > 0)
	{
		for (size_t i = 0; i < p_Node->children_count; i++)
		{
			process_node(p_Node->children[i], node, model);
		}
	}

	if (p_Node->mesh)
	{		
		for (int i = 0; i < p_Node->mesh->primitives_count; ++i)
		{
			load_primitive(p_Node, &p_Node->mesh->primitives[i], node, model.geometry_data);
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
}


static void load_textures(cgltf_texture* textures, size_t size)
{
//	std::function load_tex = [](cgltf_texture* tex, VkFormat format, bool calc_mip) -> size_t
//	{
//		const char* uri = tex->image->uri;
//		std::string name = uri ? base_path + uri : base_path + tex->image->name;
//
//		std::pair<size_t, Texture2D*> tex_object = RenderObjectManager::get_texture(name);
//
//		if (tex_object.second == nullptr)
//		{
//			if (uri)
//			{
//				/* Load from file path */
//				return RenderObjectManager::add_texture(base_path + uri, name, format, calc_mip);
//			}
//			else
//			{
//				/* Load from buffer */
//				assert(false);
//				return 0;
//			}
//		}
//		else
//		{
//			return tex_object.first;
//		}
//	};
//
//	assert(textures != nullptr);
//	
//	std::unordered_map<const char*, Image> image_datas;
//
//	for (size_t i = 0; i < size; i++)
//	{
//		cgltf_texture* tex = textures[i];
//		const char* uri = tex->image->uri;
//		std::string name = uri ? base_path + uri : base_path + tex->image->name;
//		Image im = load_image_from_file(base_path + uri);
//		image_datas.insert({ name, im });
//	}

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

bool Drawable::has_primitives() const { return RenderObjectManager::meshes[mesh_id].geometry_data.primitives.size() > 0; }
const VulkanMesh& Drawable::get_mesh() const { return RenderObjectManager::meshes[mesh_id]; }



