#include "VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/VulkanRenderDebugMarker.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

const size_t FONT_TEXTURE_INDEX = 0;

VulkanImGuiRenderer::VulkanImGuiRenderer(const VulkanContext& vkContext) 
	: VulkanRendererBase(vkContext, false)
{
	
}

constexpr uint32_t ImGuiVtxBufferSize = 64 * 1024 * sizeof(ImDrawVert);
constexpr uint32_t ImGuiIdxBufferSize = 64 * 1024 * sizeof(int);
constexpr uint32_t bufferSize = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

constexpr uint32_t numStorageBuffers   = 2;
constexpr uint32_t numUniformBuffers   = 1;
constexpr uint32_t numCombinedSamplers = 1;

void VulkanImGuiRenderer::Initialize(const std::vector<Texture2D>& textures)
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Create font texture
	m_Textures.push_back(Texture2D());

	CreateFontTexture(&io, "../../../data/fonts/BerkeleyMonoVariable-Regular.ttf", m_Textures[FONT_TEXTURE_INDEX]);
	//m_Textures[FONT_TEXTURE_INDEX].CreateView(context.device, { .format = VK_FORMAT_R8G8B8A8_UNORM, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevels = 1 });
	io.Fonts->TexID = (ImTextureID)FONT_TEXTURE_INDEX;

	// Then add other textures
	// BY DEFAULT : 
	// [0] : ImGui Font texture
	// [1] [2] : Color output of model's renderer for each swapchain image (double buffered)
	// [3] [4] : Depth output of model's renderer for each swapchain image (double buffered)

	m_ModelRendererColorTextureId[0] = 1;
	m_ModelRendererColorTextureId[1] = 2;

	m_ModelRendererDepthTextureId[0] = 3;
	m_ModelRendererDepthTextureId[1] = 4;
	
	for (size_t i = 0; i < textures.size(); i++)
	{
		m_Textures.push_back(textures[i]);
	}

	// Shaders
	m_Shader.reset(new VulkanShader("ImGui.vert.spv", "ImGui.frag.spv"));

	// Storage buffers for vertex and index data
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		CreateStorageBuffer(m_StorageBuffers[i], bufferSize);
	}

	if (!CreatePipeline(context.device))
	{
		LOG_ERROR("Failed to create pipeline for ImGui renderer.");
		assert(false);
	}
}

void VulkanImGuiRenderer::PopulateCommandBuffer(size_t currentImageIdx, VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "ImGui");

	ImDrawData* draw_data = ImGui::GetDrawData();
	UpdateBuffers(currentImageIdx, draw_data);

	int32_t width = (int32_t)context.swapchain->info.extent.width;
	int32_t height = (int32_t)context.swapchain->info.extent.height;

	VkClearValue clearValue = { { 1.0f, 0.0f, 0.0f, 1.0f } };

	BeginRenderpass(cmdBuffer, m_RenderPass, m_Framebuffers[currentImageIdx], { width, height }, &clearValue, 1);
	// Set viewport/scissor dynamically
	SetViewportScissor(cmdBuffer, width, height);

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

				// Texture ID push constant
				if (m_Textures.size() > 0)
				{
					uint32_t texture = (uint32_t)(intptr_t)pcmd->TextureId;
					vkCmdPushConstants(cmdBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &pcmd->TextureId);
				}

				vkCmdDraw(cmdBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset);
			}
		}
		idxOffset += cmdList->IdxBuffer.Size;
		vtxOffset += cmdList->VtxBuffer.Size;
	}

	EndRenderPass(cmdBuffer);
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
	UploadBufferData(m_UniformBuffers[currentImage], glm::value_ptr(inMtx), sizeof(glm::mat4), 0);
	
	// SBO : Vertex/Index data
	void* sboData = nullptr;
	vkMapMemory(context.device, m_StorageBuffers[currentImage].memory, 0, bufferSize, 0, &sboData);
	
	ImDrawVert* vtx = (ImDrawVert*)sboData;
	for (int n = 0; n < m_pDrawData->CmdListsCount; n++)
	{
		ImDrawList* cmdList = m_pDrawData->CmdLists[n];
		
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

	vkUnmapMemory(context.device, m_StorageBuffers[currentImage].memory);
}

void VulkanImGuiRenderer::LoadSceneViewTextures(Texture2D* modelRendererColor, Texture2D* modelRendererDepth)
{

}

bool VulkanImGuiRenderer::RecreateFramebuffersRenderPass()
{
	for (int i = 0; i < m_Framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(context.device, m_Framebuffers[i], nullptr);
	}

	vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
	CreateRenderPass();
	CreateFramebuffers();

	return true;
}	

VulkanImGuiRenderer::~VulkanImGuiRenderer()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_Textures[i].Destroy(context.device);
	}
}

bool VulkanImGuiRenderer::CreateFontTexture(ImGuiIO* io, const char* fontPath, Texture2D& out_Font)
{
	ImFontConfig cfg = ImFontConfig();
	cfg.FontDataOwnedByAtlas = false;
	cfg.SizePixels = 16.0f;
	
	ImFont* Font = io->Fonts->AddFontFromFileTTF(fontPath, cfg.SizePixels, &cfg);
	
	unsigned char* pixels = nullptr;
	int texWidth = 1, texHeight = 1;
	int bpp;
	io->Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight, &bpp);
	if (!pixels)
	{
		LOG_ERROR("Failed to load texture\n"); 
		return false;
	}

	out_Font.CreateFromData(
		context.device,
		pixels,
		texWidth * texHeight * bpp,
		TextureInfo{ VK_FORMAT_R8G8B8A8_UNORM, (uint32_t)texWidth, (uint32_t)texHeight, 1, 1 });
	out_Font.CreateView(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
	io->FontDefault = Font;
	//io->DisplayFramebufferScale = ImVec2(1, 1);

	return true;
}

bool VulkanImGuiRenderer::CreateRenderPass()
{
	m_RenderPassInitInfo = RenderPassInitInfo{ false, false, context.swapchain->info.colorFormat, VK_FORMAT_UNDEFINED, NONE };

	return CreateColorDepthRenderPass(m_RenderPassInitInfo, &m_RenderPass);
}

bool VulkanImGuiRenderer::CreateFramebuffers()
{
	return CreateColorDepthFramebuffers(m_RenderPass, context.swapchain.get(), m_Framebuffers.data(), false);
}

bool VulkanImGuiRenderer::CreatePipeline(VkDevice device)
{
	if (!CreateRenderPass())   return false;
	if (!CreateFramebuffers()) return false;
	if (!CreateUniformBuffers(sizeof(glm::mat4x4))) return false;
	if (!CreateDescriptorPool(numStorageBuffers, numUniformBuffers, m_Textures.size(), &m_DescriptorPool)) return false;
	if (!CreateDescriptorSets(device, m_DescriptorSets, &m_DescriptorSetLayout)) return false;
	if (!UpdateDescriptorSets(device)) return false;
	
	size_t fragConstRangeSizeInBytes = sizeof(uint32_t); // Size of 1 Push constant holding the texture ID passed from ImGui::Image
	if (!CreatePipelineLayout(device, m_DescriptorSetLayout, &m_PipelineLayout, 0, fragConstRangeSizeInBytes)) return false;
	if (!CreateGraphicsPipeline(*m_Shader.get(), false, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_RenderPass, m_PipelineLayout, &m_GraphicsPipeline, 0.0F, 0.0F, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)) return false;

	return true;
}

bool VulkanImGuiRenderer::CreateDescriptorSets(VkDevice device, VkDescriptorSet* out_DescriptorSets, VkDescriptorSetLayout* out_DescLayouts)
{
	// Specify layouts
	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },	
		{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },	// Vertex buffer
		{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },	// Index buffer
		{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = (uint32_t)(m_Textures.size()), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	return VulkanRendererBase::CreateDescriptorSets(device, bindings, out_DescriptorSets, out_DescLayouts);
}


bool VulkanImGuiRenderer::UpdateDescriptorSets(VkDevice device)
{
	// Create descriptors for each texture used in ImGui 
	std::vector<VkDescriptorImageInfo> textureDescriptors;
	for (size_t i = 0; i < m_Textures.size(); i++)
	{
		m_Textures[i].descriptor = { .sampler = s_SamplerClampNearest, .imageView = m_Textures[i].view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		textureDescriptors.push_back(m_Textures[i].descriptor);
	}

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		VkDescriptorBufferInfo uboInfo0 { .buffer = m_UniformBuffers[i].buffer, .offset = 0, .range = sizeof(glm::mat4)  };
		VkDescriptorBufferInfo sboInfo0 { .buffer = m_StorageBuffers[i].buffer, .offset = 0, .range = ImGuiVtxBufferSize };
		VkDescriptorBufferInfo sboInfo1 { .buffer = m_StorageBuffers[i].buffer, .offset = ImGuiVtxBufferSize, .range = ImGuiIdxBufferSize };

		std::array<VkWriteDescriptorSet, 4> descriptorWrites
		{
			BufferWriteDescriptorSet(m_DescriptorSets[i], 0, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 1, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(m_DescriptorSets[i], 2, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			// Descriptor array
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSets[i],
				.dstBinding = 3,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(m_Textures.size()),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = textureDescriptors.data()
			}
		};

		vkUpdateDescriptorSets(device, (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool VulkanImGuiRenderer::CreateUniformBuffers(size_t dataSizeInBytes)
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		CreateUniformBuffer(m_UniformBuffers[i], dataSizeInBytes);
	}

	return true;
}
