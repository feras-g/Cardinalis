#include "VulkanMesh.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanRendererBase.h"

#include <vector>
#include <span>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>


void VulkanMesh::create_from_file(const char* filename)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	assert(scene->HasMeshes());
	
	// Load vertices
	std::vector<VertexData> vertices;
	std::vector<unsigned int> indices;

	const aiVector3D vec3_zero { 0.0f, 0.0f, 0.0f };
	for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++)
	{
		const aiMesh* mesh = scene->mMeshes[meshIdx];

		/* vertices */
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			const aiVector3D& p = mesh->mVertices[i];
			const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : vec3_zero;
			const aiVector3D& uv = mesh->HasTextureCoords(i) ? mesh->mTextureCoords[0][i] : p; 

			vertices.push_back({ .pos = {p.x, p.y, p.z}, .normal = {n.x, n.y, n.z}, .uv = {uv.x,  uv.y} });
		}

		/* indices */
		for (size_t faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++)
		{
			const aiFace& face = mesh->mFaces[faceIdx];

			indices.push_back(face.mIndices[0]);
			indices.push_back(face.mIndices[1]);
			indices.push_back(face.mIndices[2]);
		}
	}

	m_NumVertices = vertices.size();
	m_NumIndices = indices.size();

	aiReleaseImport(scene);

	m_IdxBufferSizeInBytes = indices.size()  * sizeof(unsigned int);
	m_VtxBufferSizeInBytes = vertices.size() * sizeof(VertexData);

	CreateIndexVertexBuffer(m_vertex_index_buffer, vertices.data(), m_VtxBufferSizeInBytes, indices.data(), m_IdxBufferSizeInBytes);
}

void VulkanMesh::create_from_data(std::span<SimpleVertexData> vertices, std::span<unsigned int> indices)
{
	m_NumVertices = vertices.size();
	m_NumIndices = indices.size();
	m_IdxBufferSizeInBytes = indices.size() * sizeof(unsigned int);
	m_VtxBufferSizeInBytes = vertices.size() * sizeof(VertexData);

	CreateIndexVertexBuffer(m_vertex_index_buffer, vertices.data(), m_VtxBufferSizeInBytes, indices.data(), m_IdxBufferSizeInBytes);
}

void Drawable::draw(VkCommandBuffer cmd_buffer) const
{
	vkCmdDraw(cmd_buffer, mesh_handle->m_NumIndices, 1, 0, 0);
}

void Drawable::draw_primitives(VkCommandBuffer cmd_buffer) const
{
	for (int i = 0; i < mesh_handle->geometry_data.primitives.size(); i++)
	{
		const Primitive& p = mesh_handle->geometry_data.primitives[i];
		vkCmdDraw(cmd_buffer, p.index_count, 1, p.first_index, 0);
	}
}

Drawable::Drawable(VulkanMesh* mesh) : mesh_handle(mesh)
{

}	

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void LoadTransform(cgltf_node* p_Node, glm::mat4& currWorldMat, glm::mat4& localMat)
{
	glm::mat4 world_mat;
	cgltf_node_transform_world(p_Node, glm::value_ptr(world_mat));
	currWorldMat = world_mat;

	glm::mat4 local_mat;
	cgltf_node_transform_local(p_Node, glm::value_ptr(local_mat));
	localMat = local_mat;


	for (int i = 0; i < 16; i += 4)
	{
		////LOG_DEBUG("{0}, {1}, {2}, {3}", world_mat[i], world_mat[i + 1], world_mat[i + 2], world_mat[i + 3]);
	}
}


static void LoadVertices(cgltf_primitive* primitive, GeometryData& geometry)
{
	std::vector<glm::vec3> positionsBuffer;
	std::vector<glm::vec3> normalsBuffer;
	std::vector<glm::vec3> texCoordBuffer;
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
			positionsBuffer.resize(attribute->data->count);
			for (int i = 0; i < attribute->data->count; ++i)
			{
				cgltf_accessor_read_float(attribute->data, i, &positionsBuffer[i].x, 3);
			}

			// Also get bounding box for this primitive
			if (attribute->data->has_min)
			{
				//LOG_ERROR("Bbox min : [{0}, {1}, {2}]", attribute->data->min[0], attribute->data->min[1], attribute->data->min[2]);
			}

			if (attribute->data->has_max)
			{
				//LOG_ERROR("Bbox max : [{0}, {1}, {2}]", attribute->data->max[0], attribute->data->max[1], attribute->data->max[2]);
			}
		}
		break;

		case cgltf_attribute_type_normal:
		{
			//////LOG_DEBUG("        Normals : {0}", attribute->data->count);
			normalsBuffer.resize(attribute->data->count);
			for (int i = 0; i < normalsBuffer.size(); ++i)
			{
				cgltf_accessor_read_float(attribute->data, i, &normalsBuffer[i].x, 3);
			}
		}
		break;

		case cgltf_attribute_type_texcoord:
		{
			//////LOG_DEBUG("        Normals : {0}", attribute->data->count);
			texCoordBuffer.resize(attribute->data->count);
			for (int i = 0; i < texCoordBuffer.size(); ++i)
			{
				cgltf_accessor_read_float(attribute->data, i, &texCoordBuffer[i].x, 2);
			}
		}
		break;

		case cgltf_attribute_type_tangent:
		{
			//////LOG_DEBUG("        Tangents : {0}", attribute->data->count);
			tangentBuffer.resize(attribute->data->count);
			for (int i = 0; i < tangentBuffer.size(); ++i)
			{
				glm::vec4 t;
				cgltf_accessor_read_float(attribute->data, i, &t.x, 4);
				tangentBuffer[i] = glm::vec3(t.x, t.y, t.z) * t.w;
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
		//vertex.uv = glm::vec2(texCoordBuffer[i]);
		//vertex.tangent = tangentBuffer.empty() ? DirectX::XMFLOAT3{ 0.0, 0.0, 0.0 } : tangentBuffer[i];

		geometry.vertices.push_back(vertex);
	}

	//////LOG_DEBUG("        Vertex buffer size : {0}", st_Mesh.vertices.size());
}

static void LoadPrimitive(cgltf_primitive* primitive, GeometryData& geometry)
{
	Primitive p = {};
	{
		p.first_index = geometry.indices.size();
		p.index_count = primitive->indices->count;
	}

	for (int idx = 0; idx < p.index_count; idx++)
	{
		auto index = cgltf_accessor_read_index(primitive->indices, idx);

		geometry.indices.push_back(index + geometry.vertices.size());
	}

	geometry.primitives.push_back(p);

	LoadVertices(primitive, geometry);
}

static void ProcessGltfNode(bool bIsChild, cgltf_node* p_Node, GeometryData& geometry)
{
	if (p_Node->mesh)
	{
		glm::mat4 mesh_world_mat;
		cgltf_node_transform_world(p_Node, glm::value_ptr(mesh_world_mat));

		glm::mat4 mesh_local_mat;
		cgltf_node_transform_local(p_Node, glm::value_ptr(mesh_local_mat));

		for (int i = 0; i < p_Node->mesh->primitives_count; ++i)
		{
			LoadPrimitive(&p_Node->mesh->primitives[i], geometry);
		}
	}

	if (p_Node->children_count > 0)
	{
		for (size_t i = 0; i < p_Node->children_count; i++)
		{
			ProcessGltfNode(true, p_Node->children[i], geometry);
		}
	}
}

void VulkanMesh::create_from_file_gltf(const char* filename)
{
	cgltf_options options = { };
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, filename, &data);

	if (result == cgltf_result_success)
	{
		result = cgltf_load_buffers(&options, data, filename);

		if (result == cgltf_result_success)
		{
			for (size_t i = 0; i < data->nodes_count; ++i)
			{
				cgltf_node& currNode = data->nodes[i];

				ProcessGltfNode(false, &currNode, geometry_data);
			}
		}

		//LOG_INFO("Loaded : {0}", filename);
		////LOG_INFO("# Materials : {0}", geometry.materials.size());
		////LOG_INFO("# Textures : {0}", geometry.textures.size());
		//LOG_INFO("# primitives : {0}", geometry_data.primitives.size());

		m_NumVertices = geometry_data.vertices.size();
		m_NumIndices  = geometry_data.indices.size();

		m_IdxBufferSizeInBytes = m_NumIndices * sizeof(unsigned int);
		m_VtxBufferSizeInBytes = m_NumVertices * sizeof(VertexData);

		CreateIndexVertexBuffer(m_vertex_index_buffer, geometry_data.vertices.data(), m_VtxBufferSizeInBytes, geometry_data.indices.data(), m_IdxBufferSizeInBytes);

		cgltf_free(data);
	}
	else
	{
		LOG_ERROR("Could not read GLTF/GLB file. [error code : {0}]", result);
		assert(false);
	}
}


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

//int LoadImageFile(GeometryData* data, const std::string& rootPath, cgltf_image* image)
//{
//	char* uri = image->uri;
//
//	std::string path = rootPath + uri;
//
//	auto textureIte = data->textureTable.find(uri);
//
//	int index = -1;
//
//	if (textureIte != data->textureTable.end())
//	{
//		index = textureIte->second;
//	}
//	else
//	{
//		int width = -1;
//		int height = -1;
//		int channels = -1;
//
//		unsigned char* imgData = ImageLoader::LoadFromFile(path, width, height, channels);
//
//		// Load and store new texture
//		Texture texture
//		{
//			.info
//			{
//				.debug_name = image->name ? image->name : path.c_str(),
//				.ID = (int)data->textures.size(),
//				.width = width,
//				.height = height,
//				.channels = 4
//			},
//
//			.data = imgData
//		};
//
//		index = texture.info.ID;
//		// Update texture list
//		data->textures.push_back(texture);
//
//		data->textureTable[uri] = texture.info.ID;
//
//		////LOG_INFO("MeshLoader::LoadImageFile : Loaded {0} - {1}x{2}x{3}", path.substr(path.find_last_of('/') + 1, path.size()), width, height, channels);
//	}
//
//	return index;
//}
//
//
//void LoadMaterial(cgltf_primitive* rawPrimitive, Primitive* primitive, GeometryData* data)
//{
//	// https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/
//	// Default model used in this engine is Specular-Glossiness
//	cgltf_material* material = rawPrimitive->material;
//
//	MetallicRoughness  metallicRoughness;
//	SpecularGlossiness specularGlossiness;
//
//	MaterialProperties materialProperties;
//
//	// Load additional textures : emissive, normal maps ...
//	cgltf_texture* emissiveTexture = material->emissive_texture.texture;
//	cgltf_texture* normalTexture = material->normal_texture.texture;
//
//	if (emissiveTexture)
//	{
//		materialProperties.hEmissiveTexture = LoadImageFile(data, m_MeshRootPath, emissiveTexture->image);
//		materialProperties.hasEmissive = true;
//	}
//
//	if (normalTexture)
//	{
//		materialProperties.hNormalTexture = LoadImageFile(data, m_MeshRootPath, normalTexture->image);
//		materialProperties.hasNormalMap = true;
//	}
//
//	if (material->has_pbr_specular_glossiness)
//	{
//		////LOG_DEBUG("MATERIAL WORKFLOW: Specular-Glossiness, Name : {0}", material->name);
//
//		materialProperties.type = MaterialWorkflowType::SpecularGlossiness;
//		materialProperties.specularGlossiness =
//		{
//			.DiffuseFactor = DirectX::XMFLOAT4(material->pbr_specular_glossiness.diffuse_factor),
//			.SpecularFactor = DirectX::XMFLOAT3(material->pbr_specular_glossiness.specular_factor),
//			.GlossinessFactor = material->pbr_specular_glossiness.glossiness_factor
//		};
//
//		// Texture loading
//		cgltf_texture* diffuseTexture = material->pbr_specular_glossiness.diffuse_texture.texture;
//		cgltf_texture* specularGlossinessTexture = material->pbr_specular_glossiness.specular_glossiness_texture.texture;
//
//		if (diffuseTexture != nullptr)
//		{
//			materialProperties.specularGlossiness.hDiffuseTexture = LoadImageFile(data, m_MeshRootPath, diffuseTexture->image);
//			materialProperties.specularGlossiness.hasDiffuse = true;
//
//		}
//
//		if (specularGlossinessTexture != nullptr)
//		{
//			materialProperties.specularGlossiness.hSpecularGlossinessTexture = LoadImageFile(data, m_MeshRootPath, specularGlossinessTexture->image);
//			materialProperties.specularGlossiness.hasSpecularGlossiness = true;
//
//		}
//
//		SetMaterial(data, primitive, material, materialProperties);
//	}
//
//	else if (material->has_pbr_metallic_roughness)
//	{
//		//LOG_DEBUG("Material Workflow: Metallic/Roughness, Name : {0}", material->name ? material->name : "Unnamed");
//
//		materialProperties.type = MaterialWorkflowType::MetallicRoughness;
//		materialProperties.metallicRoughness =
//		{
//			.BaseColor = DirectX::XMFLOAT4(material->pbr_metallic_roughness.base_color_factor),
//			.Metallic = material->pbr_metallic_roughness.metallic_factor,
//			.Roughness = material->pbr_metallic_roughness.roughness_factor
//		};
//
//		// Texture loading
//		cgltf_texture* baseColorTexture = material->pbr_metallic_roughness.base_color_texture.texture;
//		cgltf_texture* metallicRoughnessTexture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
//
//		if (baseColorTexture != nullptr)
//		{
//			materialProperties.metallicRoughness.BaseColorTextureId = LoadImageFile(data, m_MeshRootPath, baseColorTexture->image);
//			materialProperties.metallicRoughness.hasBaseColorTex = true;
//		}
//
//
//		if (metallicRoughnessTexture != nullptr)
//		{
//			materialProperties.metallicRoughness.MetallicRoughnessTextureId = LoadImageFile(data, m_MeshRootPath, metallicRoughnessTexture->image);
//			materialProperties.metallicRoughness.hasMetallicRoughnessTex = true;
//		}
//
//		SetMaterial(data, primitive, material, materialProperties);
//	}
//
//
//}
//




