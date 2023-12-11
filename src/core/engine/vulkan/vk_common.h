#pragma once

#include <vulkan/vulkan.h>

#if ENGINE_DEBUG
	#include <vulkan/vk_enum_string_helper.h>
#endif

static constexpr uint32_t vk_api_version = VK_MAKE_VERSION(1, 3, 1);
