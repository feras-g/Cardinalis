#pragma once

#include "core/engine/common.h"
#include "core/engine/vulkan/objects/vk_device.h"
#include "core/engine/vulkan/objects/vk_swapchain.h"
#include "core/rendering/vulkan/vk_frame.hpp"

/* Number of frames in flight */
static constexpr uint32_t NUM_FRAMES = 2;

namespace vk
{
	struct context
	{
		VkCommandPool frames_cmd_pool;
		VkCommandPool temp_cmd_pool;

		vk::device device;
		vk::frame frames[NUM_FRAMES];

		vk::frame& get_current_frame() { return frames[curr_frame_idx]; }
		void update_frame_index() { curr_frame_idx = (curr_frame_idx + 1) % NUM_FRAMES; }

		std::unique_ptr<swapchain> swapchain;

		uint32_t frame_count = 0;
		uint32_t curr_frame_idx = 0;
		uint32_t curr_backbuffer_idx = 0;
	};

}

extern vk::context ctx;
