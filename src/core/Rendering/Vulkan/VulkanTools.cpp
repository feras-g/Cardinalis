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

Image::Image(void* data, int _w, int _h, int _n, bool b_is_float) : Image(data, b_is_float)
{
    w = _w;
    h = _h;
    n = _n;
}

Image load_image_from_file(std::string_view filename)
{
    std::string_view ext = get_extension(filename);
    
    int w, h, n;

    if (ext == "hdr")
    {
        float* data = stbi_loadf(filename.data(), &w, &h, &n, 4);
        assert(data);
        return Image{ data, w, h, n, true };
    }
    else
    {
        unsigned char* data = stbi_load(filename.data(), &w, &h, &n, 4);
        assert(data);
        return Image{ data, w, h, n, false };
    }
}

uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties)
{
    VkPhysicalDeviceMemoryProperties deviceMemProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemProperties);

    for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
    {
        if ((memoryTypeBits & (1 << i) && (deviceMemProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties))
        {
            return i;
        }
    }

    return 0;
}

std::string_view get_extension(std::string_view filename)
{
    return filename.substr(filename.find_last_of('.') + 1);
}
