#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

// Class describing a 3D model in Vulkan
class VulkanModel
{
public:
	VulkanModel() = default;

	bool CreateTexturedVertexBuffer(const char* filename);

	~VulkanModel() = default;
private:
	
};

#endif // !VULKAN_MODEL_H
