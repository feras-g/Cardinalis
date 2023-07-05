#pragma once
#include <string>
#include <sstream>
#include "Core/EngineLogger.h"
#include <vulkan/vulkan.h>

static constexpr uint64_t OneSecondInNanoSeconds = 1000000000;

#define VK_CHECK(x)                                                                             \
	do                                                                                          \
	{                                                                                           \
		VkResult err = x;                                                                       \
		if (err)                                                                                \
		{                                                                                       \
			LOG_ERROR("Detected Vulkan error {0} at {1}:{2}.\n", string_VkResult(err), __FILE__, __LINE__); \
			abort();                                                                            \
		}                                                                                       \
	} while (0)

uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties);

std::string_view get_extension(std::string_view filename);

struct Image
{
	Image(void* data, bool b_is_float);
	Image(void* data, int _w, int _h, int _n, bool b_is_float);
	float* float_data = nullptr;
	unsigned char* uchar_data = nullptr;
	inline void* get_data()
	{
		if (is_float)
		{
			return float_data;
		}
		else
		{
			return uchar_data;
		}
	}

	int w{}, h{}, n{};
	bool is_float = false;

	~Image() 
	{
		if (uchar_data) {
			delete uchar_data; 
			uchar_data = nullptr;
		}
		if (float_data) {
			delete float_data;
			float_data = nullptr;
		}
	}
};

Image load_image_from_file(std::string_view filename);

#define CHECK_DEREF(p) { assert(p); return *p; }
