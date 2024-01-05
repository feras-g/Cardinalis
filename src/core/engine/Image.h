#pragma once
#include <string>
#include <sstream>
#include "core/engine/logger.h"
#include <vulkan/vulkan.h>

std::string_view get_extension(std::string_view filename);

class Image
{
public:
	Image() = default;
	Image(void* data, bool b_is_float);
	Image(void* data, int _w, int _h, bool b_is_float);
	Image& operator=(const Image& other);

	~Image();
	void* get_data() const;
	int w = 0;
	int h = 0;

	void load_from_buffer(unsigned char const* buffer, size_t buffer_size);
	void load_from_file(std::string_view filename);
	int data_size_bytes = -1;
private:
	float* float_data = nullptr;
	unsigned char* uchar_data = nullptr;

	bool is_float = false;
};