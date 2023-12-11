#include "vk_texture_job.h"

namespace vk
{
	texture_job::texture_job(vk::texture* texture, texture_load_info load_info)
		: status(job_status::PENDING), texture(texture), load_info(load_info)
	{

	}

	void texture_job::execute(VkCommandBuffer cmd_buffer)
	{
		texture->load(load_info);
		status = job_status::COMPLETE;
	}
}

