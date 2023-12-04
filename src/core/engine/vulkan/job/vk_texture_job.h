#pragma once

#include "vk_job.h"
#include "core/engine/vulkan/objects/vk_texture.h"

namespace vk
{
	struct texture_load_info
	{
		const char* filepath;
		VkCommandBuffer cmd_buffer;
		VkQueue queue;
	};

	class texture_job : public job
	{
	public:
		texture_job(texture* texture, texture_load_info load_info);

		texture *texture;
		texture_load_info load_info;
		job_status status;

		virtual void execute(VkCommandBuffer cmd_buffer) final override;
	};
}
