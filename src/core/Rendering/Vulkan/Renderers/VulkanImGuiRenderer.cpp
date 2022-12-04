#include "VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"
#include "glm/mat4x4.hpp"
#include "imgui/imgui.h"

VulkanImGuiRenderer::VulkanImGuiRenderer(const VulkanContext& vkContext) 
	: VulkanRendererBase(vkContext)
{
}

constexpr uint32_t ImGuiVtxBufferSize = 64 * 1024 * sizeof(ImDrawVert);
constexpr uint32_t ImGuiIdxBufferSize = 64 * 1024 * sizeof(int);
constexpr uint32_t bufferSize = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

constexpr uint32_t numStorageBuffers   = NUM_FRAMES;
constexpr uint32_t numUniformBuffers   = 1;
constexpr uint32_t numCombinedSamplers = 1;


void VulkanImGuiRenderer::Initialize()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	CreateFontTexture(&io, "../../../data/fonts/Roboto-Medium.ttf", m_FontTexture);
	m_FontTexture.CreateImageView(context.device, { .format = VK_FORMAT_R8G8B8A8_UNORM, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevels = 1 });
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, m_FontSampler);

	// 1 storage buffer for each frame
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		if (!CreateBuffer(ImGuiVtxBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_StorageBuffers[i], m_StorageBufferMems[i]))
		{
			LOG_ERROR("Failed to create storage buffers for ImGui Renderer.");
			assert(false);
		}
	}

	CreatePipeline(context.device);
}

void VulkanImGuiRenderer::PopulateCommandBuffer(VkCommandBuffer cmdBuffer) const
{
	//VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Pass: ImGui");

}

VulkanImGuiRenderer::~VulkanImGuiRenderer()
{
}

bool VulkanImGuiRenderer::CreateFontTexture(ImGuiIO* io, const char* fontPath, VulkanTexture& out_Font)
{
	ImFontConfig cfg = ImFontConfig();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply = 1.5f;
	cfg.SizePixels = 768.0f / 32.0f;
	cfg.PixelSnapH = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;
	ImFont* Font = io->Fonts->AddFontFromFileTTF(fontPath, cfg.SizePixels, &cfg);

	unsigned char* pixels = nullptr;
	int texWidth = 1, texHeight = 1;
	io->Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

	//vkDev, textureImage, textureImageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM
	if (!pixels || !out_Font.CreateImageFromData(context.mainCmdBuffer, context.device, { (unsigned int)texWidth, (unsigned int)texHeight, VK_FORMAT_R8G8B8A8_UNORM }, pixels))
	{
		LOG_ERROR("Failed to load texture\n"); 
		return false;
	}

	io->Fonts->TexID = (ImTextureID)0;
	io->FontDefault = Font;
	io->DisplayFramebufferScale = ImVec2(1, 1);

	return true;
}

bool VulkanImGuiRenderer::CreateTextureSampler(VkDevice device, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode, VkSampler& out_Sampler)
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.flags = 0,
		.magFilter = magFilter,	// VK_FILTER_LINEAR, VK_FILTER_NEAREST
		.minFilter = minFilter, // VK_FILTER_LINEAR, VK_FILTER_NEAREST
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, // VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST
		.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	return (vkCreateSampler(context.device, &samplerInfo, nullptr, &out_Sampler) == VK_SUCCESS);
}

bool VulkanImGuiRenderer::CreatePipeline(VkDevice device)
{
	CreateColorDepthRenderPass({ false, false, NONE }, true, &m_RenderPass);
	CreateDescriptorPool(numStorageBuffers, numUniformBuffers, numCombinedSamplers, &m_DescriptorPool);
	CreateDescriptorSets(context.device, m_DescriptorSets, &m_DescriptorSetLayout);
	CreatePipelineLayout(context.device, m_DescriptorSetLayout, &m_PipelineLayout);

	return true;
}

bool VulkanImGuiRenderer::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	// Specify layouts
	constexpr uint32_t bindingCount = 4;
	VkDescriptorSetLayoutBinding bindings[bindingCount] =
	{
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{ .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{ .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{ .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = 0,
		.bindingCount = bindingCount,
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	std::array<VkDescriptorSetLayout, NUM_FRAMES> layouts = { m_DescriptorSetLayout, m_DescriptorSetLayout };	// 1 descriptor set per frame

	// Allocate memory
	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_DescriptorPool,
		.descriptorSetCount = NUM_FRAMES,
		.pSetLayouts = layouts.data(),
	};

	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets));

	// Update contents
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		VkDescriptorBufferInfo sboInfo0   = { .buffer = m_StorageBuffers[i], .offset = 0, .range = ImGuiVtxBufferSize };
		VkDescriptorBufferInfo sboInfo1   = { .buffer = m_StorageBuffers[i], .offset = ImGuiVtxBufferSize, .range = ImGuiIdxBufferSize };
		VkDescriptorBufferInfo uboInfo0   = { .buffer = m_UniformBuffers[i], .offset = 0, .range = sizeof(glm::mat4)  };
		VkDescriptorImageInfo imageInfo0  = { .sampler = m_FontSampler, .imageView = m_FontTexture.view, .imageLayout = m_FontTexture.info.layout };

		std::array<VkWriteDescriptorSet, 4> descriptorWrites =
		{
			BufferWriteDescriptorSet(m_DescriptorSets[i], 0, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 1, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 2, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			ImageWriteDescriptorSet (m_DescriptorSets[i], 3,  &imageInfo0)
		};

		vkUpdateDescriptorSets(device, (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
	//return (vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets) == VK_SUCCESS);
}

bool VulkanImGuiRenderer::CreatePipelineLayout(VkDevice device, VkDescriptorSetLayout descSetLayout, VkPipelineLayout* out_PipelineLayout)
{
	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &descSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, out_PipelineLayout));

	return true;
}
