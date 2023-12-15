#pragma once

#include "core/engine/common.h"

#include "core/rendering/vulkan/"
struct directional_light
{
	glm::vec4 dir;
	glm::vec4 color;
};

struct lighting
{
	Buffer ubo;
};

