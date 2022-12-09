#include "VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


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

static void ConvertColorPacked_sRGBToLinearRGB(uint32_t& color)
{
	// Unpack
	float s = 1.0f / 255.0f;
	float gamma = 2.2;

	float r = ((color >> 0)  & 0xFF) * s;
	float g = ((color >> 8)  & 0xFF) * s;
	float b = ((color >> 16) & 0xFF) * s;
	float a = ((color >> 24) & 0xFF) * s;

	// Convert to linear space
	r = pow(r, gamma);
	g = pow(g, gamma);
	b = pow(b, gamma);

	// Pack
	color = ((uint8_t)(r * 255.0f) << 0  |
			 (uint8_t)(g * 255.0f) << 8  |
			 (uint8_t)(b * 255.0f) << 16 |
			 (uint8_t)(a * 255.0f) << 24);
}

void VulkanImGuiRenderer::Initialize()
{
	ImGuiIO& io = ImGui::GetIO();

	// Textures
	CreateFontTexture(&io, "../../../data/fonts/ProggyClean.ttf", m_FontTexture);
	m_FontTexture.CreateImageView(context.device, { .format = VK_FORMAT_R8G8B8A8_UNORM, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevels = 1 });
	CreateTextureSampler(context.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, m_FontSampler);

	// Shaders
	m_Shader.reset(new VulkanShader("ImGui.vert.spv", "ImGui.frag.spv"));

	// Storage buffers for vertex and index data
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		if (!CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_StorageBuffers[i], m_StorageBufferMems[i]))
		{
			LOG_ERROR("Failed to create storage buffers for ImGui Renderer.");
			assert(false);
		}
	}

	if (!CreatePipeline(context.device))
	{
		LOG_ERROR("Failed to create pipeline for ImGui renderer.");
		assert(false);
	}
}

void VulkanImGuiRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) const
{
	//VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "Pass: ImGui");

	int32_t width  = (int32_t)context.swapchain->info.extent.width;
	int32_t height = (int32_t)context.swapchain->info.extent.height;

	VkRenderPassBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_RenderPass,
		.framebuffer = m_Framebuffers[currentImageIdx],
		.renderArea = { width, height },
		.clearValueCount = 0,
		.pClearValues = nullptr
	};
	vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentImageIdx], 0, nullptr);

	ImVec2 clipOff = m_pDrawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clipScale = m_pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	int vtxOffset = 0;
	int idxOffset = 0;

	for (int n = 0; n < m_pDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = m_pDrawData->CmdLists[n];

		for (int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++)
		{
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];
			
			if (pcmd->UserCallback)
				return;

			// Project scissor/clipping rectangles into framebuffer space
			ImVec4 clipRect;
			clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
			clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
			clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
			clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

			if (clipRect.x < width && clipRect.y < height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
			{
				if (clipRect.x < 0.0f) clipRect.x = 0.0f;
				if (clipRect.y < 0.0f) clipRect.y = 0.0f;

				// Apply scissor/clipping rectangle
				const VkRect2D scissor = {
					.offset = {.x = (int32_t)(clipRect.x), .y = (int32_t)(clipRect.y) },
					.extent = {.width = (uint32_t)(clipRect.z - clipRect.x), .height = (uint32_t)(clipRect.w - clipRect.y) }
				};
				vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

				vkCmdDraw(cmdBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset);
			}
		}
		idxOffset += cmdList->IdxBuffer.Size;
		vtxOffset += cmdList->VtxBuffer.Size;
	}

	vkCmdEndRenderPass(cmdBuffer);
}

void VulkanImGuiRenderer::UpdateBuffers(size_t currentImage, ImDrawData* pDrawData)
{
	m_pDrawData = pDrawData;

	const float L = m_pDrawData->DisplayPos.x;
	const float R = m_pDrawData->DisplayPos.x + m_pDrawData->DisplaySize.x;
	const float T = m_pDrawData->DisplayPos.y;
	const float B = m_pDrawData->DisplayPos.y + m_pDrawData->DisplaySize.y;

	// UBO
	const glm::mat4 inMtx = glm::ortho(L, R, T, B);
	UploadBufferData(m_UniformBuffersMemory[currentImage], 0, glm::value_ptr(inMtx), sizeof(glm::mat4));
	
	// SBO : Vertex/Index data
	void* sboData = nullptr;
	vkMapMemory(context.device, m_StorageBufferMems[currentImage], 0, bufferSize, 0, &sboData);
	
	ImDrawVert* vtx = (ImDrawVert*)sboData;
	for (int n = 0; n < m_pDrawData->CmdListsCount; n++)
	{
		ImDrawList* cmdList = m_pDrawData->CmdLists[n];
		
		for (int i = 0; i < cmdList->VtxBuffer.Size; i++)
		{
			ConvertColorPacked_sRGBToLinearRGB(cmdList->VtxBuffer[i].col);
		}

		memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		vtx += cmdList->VtxBuffer.Size;
	}

	uint32_t* idx = (uint32_t*)((uint8_t*)sboData + ImGuiVtxBufferSize);
	for (int n = 0; n < m_pDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = m_pDrawData->CmdLists[n];
		const uint16_t* src = (const uint16_t*)cmdList->IdxBuffer.Data;

		for (int j = 0; j < cmdList->IdxBuffer.Size; j++)
		{
			*idx++ = (uint32_t)*src++;
		}
	}

	vkUnmapMemory(context.device, m_StorageBufferMems[currentImage]);
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
	if (!CreateColorDepthRenderPass({ false, false, NONE }, false, &m_RenderPass)) return false;
	if (!CreateColorDepthFramebuffers(m_RenderPass,  context.swapchain.get(), m_Framebuffers, false)) return false;
	if (!CreateUniformBuffers(sizeof(glm::mat4x4))) return false;
	if (!CreateDescriptorPool(numStorageBuffers, numUniformBuffers, numCombinedSamplers, &m_DescriptorPool)) return false;
	if (!CreateDescriptorSets(context.device, m_DescriptorSets, &m_DescriptorSetLayout)) return false;
	if (!CreatePipelineLayout(context.device, m_DescriptorSetLayout, &m_PipelineLayout)) return false;
	if (!CreateGraphicsPipeline(*m_Shader.get(), false, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_RenderPass, m_PipelineLayout, &m_GraphicsPipeline)) return false;
	
	return true;
}

bool VulkanImGuiRenderer::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	// Specify layouts
	constexpr uint32_t bindingCount = 4;
	VkDescriptorSetLayoutBinding bindings[bindingCount] =
	{
		{ .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{ .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
		{ .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
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
		VkDescriptorBufferInfo uboInfo0   = { .buffer = m_UniformBuffers[i], .offset = 0, .range = sizeof(glm::mat4)  };
		VkDescriptorBufferInfo sboInfo0   = { .buffer = m_StorageBuffers[i], .offset = 0, .range = ImGuiVtxBufferSize };
		VkDescriptorBufferInfo sboInfo1   = { .buffer = m_StorageBuffers[i], .offset = ImGuiVtxBufferSize, .range = ImGuiIdxBufferSize };
		VkDescriptorImageInfo imageInfo0  = { .sampler = m_FontSampler, .imageView = m_FontTexture.view, .imageLayout = m_FontTexture.info.layout };

		std::array<VkWriteDescriptorSet, 4> descriptorWrites =
		{
			BufferWriteDescriptorSet(m_DescriptorSets[i], 0, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 1, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 2, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
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

bool VulkanImGuiRenderer::CreateUniformBuffers(size_t dataSizeInBytes)
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		if (!CreateUniformBuffer(dataSizeInBytes, m_UniformBuffers[i], m_UniformBuffersMemory[i]))
		{
			LOG_ERROR("Failed to create uniform buffer.");
			return false;
		}
	}

	return true;
}
