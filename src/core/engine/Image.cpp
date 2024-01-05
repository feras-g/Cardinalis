#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Image.h"

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

Image& Image::operator=(const Image& other)
{
    if (&other != this)
    {
        this->w = other.w;
        this->h = other.h;

        if (is_float)
        {
            this->is_float = true;
            this->float_data = other.float_data;
        }
        else
        {
            this->is_float = false;
            this->uchar_data = other.uchar_data;
        }
    }

    return *this;
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

void Image::load_from_file(std::string_view filename)
{
    std::string_view ext = get_extension(filename);
 
    int n;

    if (ext == "hdr")
    {
        is_float = true;
        float_data = stbi_loadf(filename.data(), &w, &h, &n, 4);
        assert(float_data);
    }
    else
    {
        is_float = false;
        uchar_data = stbi_load(filename.data(), &w, &h, &n, 4);
        assert(uchar_data);
    }
}

void Image::load_from_buffer(stbi_uc const* buffer, size_t buffer_size)
{
    int n;
    unsigned char* data = stbi_load_from_memory(buffer, (int)buffer_size, &w, &h, &n, 4);
    assert(data);
    LOG_DEBUG("Image load from buffer: Dimensions: {1}x{2}x{3} Size: {0}", buffer_size, w, h, n);
    uchar_data = data;
    is_float = false;
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
