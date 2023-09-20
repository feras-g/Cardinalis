#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "VulkanTools.h"

Image::Image(void* imdata, bool b_is_float)
{
    is_float = b_is_float;

    if (is_float)
    {
        float_data = static_cast<float*>(imdata);
    }
    else
    {
        uchar_data = static_cast<unsigned char*>(imdata);
    }
}

Image::Image(void* data, int _w, int _h, bool b_is_float) : Image(data, b_is_float)
{
    w = _w;
    h = _h;
}

void* Image::get_data() const
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

Image load_image_from_file(std::string_view filename)
{
    std::string_view ext = get_extension(filename);
    
    int w, h, n;

    if (ext == "hdr")
    {
        float* data = stbi_loadf(filename.data(), &w, &h, &n, 4);
        assert(data);
        return Image{ data, w, h,true };
    }
    else
    {
        unsigned char* data = stbi_load(filename.data(), &w, &h, &n, 4);
        assert(data);
        return Image{ data, w, h, false };
    }
}

std::string_view get_extension(std::string_view filename)
{
    return filename.substr(filename.find_last_of('.') + 1);
}

Image::~Image()
{
	if (uchar_data)
	{
		delete uchar_data;
		uchar_data = nullptr;
	}
	if (float_data)
	{
		delete float_data;
		float_data = nullptr;
	}
}
