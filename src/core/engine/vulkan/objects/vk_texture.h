#pragma once


namespace vk
{
	struct texture_load_info;

	class texture
	{
	public:
		texture() = default;
		void load(const texture_load_info& load_info);
	};
}
