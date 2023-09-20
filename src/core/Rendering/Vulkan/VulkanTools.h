#pragma once
#include <string>
#include <sstream>
#include "core/engine/EngineLogger.h"
#include <vulkan/vulkan.h>

std::string_view get_extension(std::string_view filename);

class Image
{
public:
	Image(void* data, bool b_is_float);
	Image(void* data, int _w, int _h, bool b_is_float);
	~Image();
	void* get_data() const;
	int w = 0;
	int h = 0;
private:
	float* float_data = nullptr;
	unsigned char* uchar_data = nullptr;

	bool is_float = false;
};

Image load_image_from_file(std::string_view filename);
