#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "VulkanTools.h"

Image load_image_from_file(std::string_view filename)
{
int w, h, n;
stbi_uc* data = stbi_load(filename.data(), &w, &h, &n, 4);
assert(data);
return { ImageData(data, free), w, h, n };
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