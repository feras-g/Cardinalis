#pragma once

#include "core/engine/vulkan/vk_common.h"

namespace vk
{
	enum class job_status
	{
		PENDING,
		IN_PROGRESS,
		COMPLETE,
	};

	class job
	{
	public:
		job_status status = job_status::PENDING;
		virtual void execute(VkCommandBuffer cmd_buffer) = 0;
	};
}


