#include "VulkanModel.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"

#include <vector>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Vertex description
struct VertexData
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

bool VulkanModel::CreateFromFile(const char* filename)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	assert(scene->HasMeshes());

	const aiMesh* mesh = scene->mMeshes[0];
	
	// Load vertices
	std::vector<VertexData> vertices;
	for (size_t i=0; i < mesh->mNumVertices; i++)
	{
		const aiVector3D& p = mesh->mVertices[i];

		const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D{0.0f, 0.0f, 0.0f} ;
		
		const aiVector3D& uv = mesh->HasTextureCoords(i) ? mesh->mTextureCoords[0][i] : p; // Select first uv channel by default

		vertices.push_back({ .pos = {p.x, p.y, p.z}, .normal = {n.x, n.y, n.z}, .uv = {uv.x,  1.0f - uv.y}});
	}
	m_NumVertices = vertices.size();

	// Load indices
	std::vector<unsigned int> indices;
	for (size_t faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++)
	{
		const aiFace& f = mesh->mFaces[faceIdx];
		
		for (int vertexIdx = 0; vertexIdx < 3; vertexIdx++) 
		{ 
			indices.push_back(f.mIndices[vertexIdx]); 
		};
	}
	m_NumIndices = indices.size();

	aiReleaseImport(scene);

	m_IdxBufferSizeInBytes = indices.size()  * sizeof(unsigned int);
	m_VtxBufferSizeInBytes = vertices.size() * sizeof(VertexData);

	CreateIndexVertexBuffer(m_StorageBuffer, vertices.data(), m_VtxBufferSizeInBytes, indices.data(), m_IdxBufferSizeInBytes);

	return true;
}
