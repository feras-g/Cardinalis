#include "VulkanModel.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Vertex description
struct VertexData
{
	struct
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};
};

bool VulkanModel::CreateTexturedVertexBuffer(const char* filename)
{
	return false;
}
