#pragma once

#include <vector>
#include <unordered_map>
#include <glm/mat4x4.hpp>

struct SceneNode
{
	int meshIdx;
	int materialIdx;
	int parentIdx;
	int firstChildIdx;
	int rightSiblingIdx;
};

struct Hierarchy
{
	int parent;
	int firstChild;
	int nextSibling;
	int depth;
};

struct Scene
{
	std::vector<Hierarchy> hierarchy;
	std::vector<glm::mat4> localTransform;
	std::vector<glm::mat4> globalTransform;

	// Node to Mesh mapping
	std::unordered_map<uint32_t, uint32_t> meshForNode;
	// Node to Material mapping
	std::unordered_map<uint32_t, uint32_t> materialForNode;
	// Node to Node labels mapping
	std::unordered_map<uint32_t, uint32_t> labelForNode;

	std::vector<std::string> nodeLabels;
	std::vector<std::string> materialLabels;
};