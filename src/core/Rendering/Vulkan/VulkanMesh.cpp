#include "VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"
#include "VulkanTools.h"

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
	LOG_INFO("Filename extension: {}", ext);
	
	if (ext == "glb" || ext == "gltf")
	{
		create_from_file_gltf(filename);
	}
	else
	{
		assert(false);
	}
//	else
//	{
//		const aiScene* scene = aiImportFile(filename.data(), aiProcess_Triangulate);
//
//		assert(scene->HasMeshes());
//
//		// Load vertices
//		std::vector<VertexData> vertices;
//		std::vector<unsigned int> indices;
//
//		const aiVector3D vec3_zero{ 0.0f, 0.0f, 0.0f };
//		for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++)
//		{
//			const aiMesh* mesh = scene->mMeshes[meshIdx];
//
//			/* vertices */
//			for (size_t i = 0; i < mesh->mNumVertices; i++)
//			{
//				const aiVector3D& p = mesh->mVertices[i];
//				const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : vec3_zero;
//				const aiVector3D& uv = mesh->HasTextureCoords(i) ? mesh->mTextureCoords[0][i] : p;
//
//				vertices.push_back({ .pos = {p.x, p.y, p.z}, .normal = {n.x, n.y, n.z}, .uv = {uv.x,  uv.y} });
//			}
//
//			/* indices */
//			for (size_t faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++)
//			{
//				const aiFace& face = mesh->mFaces[faceIdx];
//
//				indices.push_back(face.mIndices[0]);
//				indices.push_back(face.mIndices[1]);
//				indices.push_back(face.mIndices[2]);
//			}
//		}
//
//		m_num_vertices = vertices.size();
//		m_num_indices = indices.size();
//
//		aiReleaseImport(scene);
//
//		m_index_buf_size_bytes = indices.size() * sizeof(unsigned int);
//		m_vertex_buf_size_bytes = vertices.size() * sizeof(VertexData);
//
//		create_vertex_index_buffer(m_vertex_index_buffer, vertices.data(), m_vertex_buf_size_bytes, indices.data(), m_index_buf_size_bytes);
//	}
//
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

void Drawable::draw_primitives(VkCommandBuffer cmd_buffer) const
{
	VulkanMesh& mesh = RenderObjectManager::meshes[mesh_id];
	for (int i = 0; i < mesh.geometry_data.primitives.size(); i++)
	{
		const Primitive& p = mesh.geometry_data.primitives[i];
		vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
	}
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
			//////LOG_DEBUG("        Positions : {0}", attribute->data->count);
			//positionsBuffer.resize(attribute->data->count);
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
				LOG_ERROR("Bbox min : [{0}, {1}, {2}]", geometry.bbox_min_WS[0], geometry.bbox_min_WS[1], geometry.bbox_min_WS[2]);
			}

			if (attribute->data->has_max)
			{
				glm::vec4 bbox_max_OS(attribute->data->max[0], attribute->data->max[1], attribute->data->max[2], 1.0f);
				geometry.bbox_max_WS = bbox_max_OS * geometry.world_mat;
				LOG_ERROR("Bbox max : [{0}, {1}, {2}]", geometry.bbox_max_WS[0], geometry.bbox_max_WS[1], geometry.bbox_max_WS[2]);
			}
		}
		break;

		case cgltf_attribute_type_normal:
		{
			//////LOG_DEBUG("        Normals : {0}", attribute->data->count);
			//normalsBuffer.resize(attribute->data->count);
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
			//////LOG_DEBUG("        Normals : {0}", attribute->data->count);
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
			//////LOG_DEBUG("        Tangents : {0}", attribute->data->count);
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

	//////LOG_DEBUG("        Vertex buffer size : {0}", st_Mesh.vertices.size());
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
		//std::string material_name = "Unnamed Material " + std::to_string(RenderObjectManager::materials.size());
		/*
			Base Color
			Normal
			MetallicRoughness
			Emissive
		*/

		std::function load_tex = [](cgltf_texture* tex, VkFormat format, bool calc_mip) -> size_t
		{
			const char* uri = tex->image->uri;
			const char* name = uri ? uri : tex->image->name;

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
		cgltf_texture* tex_emissive = gltf_mat->emissive_texture.texture;

		if (tex_normal)
		{
			material.tex_normal_id = (unsigned int)load_tex(tex_normal, tex_normal_format, true);
		}

		if (tex_emissive)
		{
			material.tex_emissive_id = (unsigned int)load_tex(tex_emissive, tex_emissive_format, true);
		}

		if (gltf_mat->has_pbr_metallic_roughness)
		{
			//LOG_DEBUG("Material Workflow: Metallic/Roughness, Name : {0}", gltf_material->name ? gltf_material->name : "Unnamed");

			cgltf_texture* tex_base_color = gltf_mat->pbr_metallic_roughness.base_color_texture.texture;
			cgltf_texture* tex_metallic_roughness = gltf_mat->pbr_metallic_roughness.metallic_roughness_texture.texture;

			if (tex_base_color)
			{
				material.tex_base_color_id = (unsigned int)load_tex(tex_base_color, tex_base_color_format, true);
			}

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

		if (gltf_mat->has_pbr_specular_glossiness)
		{
			assert(false);

			////LOG_DEBUG("MATERIAL WORKFLOW: Specular-Glossiness, Name : {0}", material->name);

			//materialProperties.type = MaterialWorkflowType::SpecularGlossiness;
			//materialProperties.specularGlossiness =
			//{
			//	.DiffuseFactor = DirectX::XMFLOAT4(gltf_mat->pbr_specular_glossiness.diffuse_factor),
			//	.SpecularFactor = DirectX::XMFLOAT3(gltf_mat->pbr_specular_glossiness.specular_factor),
			//	.GlossinessFactor = gltf_mat->pbr_specular_glossiness.glossiness_factor
			//};

			//// Texture loading
			//cgltf_texture* diffuseTexture = gltf_mat->pbr_specular_glossiness.diffuse_texture.texture;
			//cgltf_texture* specularGlossinessTexture = gltf_mat->pbr_specular_glossiness.specular_glossiness_texture.texture;

			//if (diffuseTexture != nullptr)
			//{
			//	materialProperties.specularGlossiness.hDiffuseTexture = LoadImageFile(data, m_MeshRootPath, diffuseTexture->image);
			//	materialProperties.specularGlossiness.hasDiffuse = true;

			//}

			//if (specularGlossinessTexture != nullptr)
			//{
			//	materialProperties.specularGlossiness.hSpecularGlossinessTexture = LoadImageFile(data, m_MeshRootPath, specularGlossinessTexture->image);
			//	materialProperties.specularGlossiness.hasSpecularGlossiness = true;

			//}

			//SetMaterial(data, primitive, gltf_mat, materialProperties);
		}

		std::size_t hash = MyHash<Material>{}(material);
		//auto res = material_hashes.insert(hash);
		//if (res.second == false)
		/*if (gltf_mat->name)
		{
			material_name = gltf_mat->name;
		}
		else
		{
			assert(false);
		}*/
		
		std::string material_name = std::to_string(hash);
		LOG_DEBUG("Loaded new material named {0}, workflow : (Metallic/Roughness)", material_name);

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

static void load_primitive(cgltf_node* node, cgltf_primitive* primitive, GeometryData& geometry)
{
	Primitive p = {};
	p.first_index = (uint32_t)geometry.indices.size();
	p.index_count = (uint32_t)primitive->indices->count;
	 
	//glm::mat4 mesh_world_mat;
	//cgltf_node_transform_world(node, glm::value_ptr(mesh_world_mat));
	//p.mat_model *= mesh_world_mat;

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
	//p.material_id = 0;
	geometry.primitives.push_back(p);
}

static void process_node(bool bIsChild, cgltf_node* p_Node, GeometryData& geometry)
{
	if (p_Node->mesh)
	{		
		for (int i = 0; i < p_Node->mesh->primitives_count; ++i)
		{
			load_primitive(p_Node, &p_Node->mesh->primitives[i], geometry);
		}
	}

	if (p_Node->children_count > 0)
	{
		for (size_t i = 0; i < p_Node->children_count; i++)
		{
			process_node(true, p_Node->children[i], geometry);
		}
	}
}


void VulkanMesh::create_from_file_gltf(const std::string& filename)
{
	cgltf_options options = { };
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, filename.c_str(), &data);

	auto const pos = filename.find_last_of('/');

	base_path = filename.substr(0, pos + 1);

	if (result == cgltf_result_success)
	{
		result = cgltf_load_buffers(&options, data, filename.c_str());

		
		LOG_INFO("Start loading {}", filename);
		LOG_INFO("# Materials : {}", data->materials_count);
		LOG_INFO("# Textures : {}",  data->textures_count);
		LOG_INFO("# Meshes : {}", data->meshes_count);

		if (result == cgltf_result_success)
		{
			for (size_t i = 0; i < data->nodes_count; ++i)
			{
				cgltf_node& currNode = data->nodes[i];

				process_node(false, &currNode, geometry_data);
			}
		}



		m_num_vertices = geometry_data.vertices.size();
		m_num_indices  = geometry_data.indices.size();

		m_index_buf_size_bytes = m_num_indices * sizeof(unsigned int);
		m_vertex_buf_size_bytes = m_num_vertices * sizeof(VertexData);

		model = geometry_data.world_mat;

		create_from_data(geometry_data.vertices, geometry_data.indices);

		cgltf_free(data);
	}
	else
	{
		LOG_ERROR("Could not read GLTF/GLB file. [error code : {0}]", result);
		assert(false);
	}
}


bool Drawable::has_primitives() const { return RenderObjectManager::meshes[mesh_id].geometry_data.primitives.size() > 0; }
const VulkanMesh& Drawable::get_mesh() const { return RenderObjectManager::meshes[mesh_id]; }

//void SetMaterial(GeometryData* data, Primitive* primitive, cgltf_material* rawMaterial, MaterialProperties& material)
//{
//	auto materialIte = data->materialTable.find(rawMaterial->name);
//
//	if (rawMaterial->name != nullptr && materialIte != data->materialTable.end())
//	{
//		primitive->MaterialName = materialIte->first;
//		primitive->MaterialId = materialIte->second;
//	}
//	else
//	{
//		// Insert new material
//		int materialId = (int)data->materials.size();
//
//		std::string name = std::string("Material ").append(std::to_string(materialId));
//
//		material.Id = materialId;
//		material.name = name;
//
//		primitive->MaterialName = material.name;
//		primitive->MaterialId = materialId;
//
//		data->materialTable.insert({ name.c_str(), materialId });
//		data->materials.push_back(material);
//	}
//}






