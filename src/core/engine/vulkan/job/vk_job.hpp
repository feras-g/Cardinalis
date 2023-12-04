#pragma once

#include "core/engine/vulkan/vk_common.h"

enum class JobStatus
{
	PENDING,
	IN_PROGRESS,
	COMPLETE,
};

class Job
{
public:
	JobStatus status = JobStatus::PENDING;
	virtual void execute(VkCommandBuffer cmd_buffer) = 0;
};

