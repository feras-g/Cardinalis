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

		/* Vertices */
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			const aiVector3D& p = mesh->mVertices[i];
			const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : vec3_zero;
			const aiVector3D& uv = mesh->HasTextureCoords(i) ? mesh->mTextureCoords[0][i] : p; 

			vertices.push_back({ .pos = {p.x, p.y, p.z}, .normal = {n.x, n.y, n.z}, .uv = {uv.x,  uv.y} });
		}

		/* Indices */
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
	vkCmdDraw(cmd_buffer, mesh_handle->m_NumVertices, 1, 0, 0);
}


Drawable::Drawable(VulkanMesh* mesh, glm::mat4 model) : mesh_handle(mesh), model(model)
{

}
