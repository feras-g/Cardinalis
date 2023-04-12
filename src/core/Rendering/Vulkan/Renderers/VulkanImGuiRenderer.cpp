#include "VulkanImGuiRenderer.h"
#include "Rendering/Vulkan/VulkanDebugUtils.h"
#include "Rendering/Vulkan/VulkanRenderInterface.h"
#include "Rendering/Vulkan/Renderers/VulkanModelRenderer.h"
#include "Rendering/Vulkan/Renderers/DeferredRenderer.h"
#include "Rendering/Vulkan/VulkanShader.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"

const size_t FONT_TEXTURE_INDEX = 0;

VulkanImGuiRenderer::VulkanImGuiRenderer(const VulkanContext& vkContext)
{
}

constexpr uint32_t ImGuiVtxBufferSize = 64 * 1024 * sizeof(ImDrawVert);
constexpr uint32_t ImGuiIdxBufferSize = 64 * 1024 * sizeof(int);
constexpr uint32_t imgui_buffer_size = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

constexpr uint32_t numStorageBuffers   = 2;
constexpr uint32_t numUniformBuffers   = 1;
constexpr uint32_t numCombinedSamplers = 1;

void VulkanImGuiRenderer::Initialize(const VulkanModelRenderer& model_renderer, const DeferredRenderer& deferred_renderer)
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Create font texture
	m_Textures.push_back(Texture2D());

	CreateFontTexture(&io, "../../../data/fonts/Arial-Bold.ttf", m_Textures[FONT_TEXTURE_INDEX]);
	//m_Textures[FONT_TEXTURE_INDEX].CreateView(context.device, { .format = VK_FORMAT_R8G8B8A8_UNORM, .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevels = 1 });
	io.Fonts->TexID = (ImTextureID)FONT_TEXTURE_INDEX;

	// Then add other textures
	// BY DEFAULT : 
	// [0] : ImGui Font texture
	// [1] [2] : Output of deferred renderer for Frame 0 / Frame 1
	// [3] [4] : Color output of model's renderer for Frame 0 / Frame 1
	// [5] [6] : Normal output of model's renderer for Frame 0 / Frame 1
	// [7] [8] : Depth output of model's renderer for Frame 0 / Frame 1

	int tex_id = 0;
	for (size_t i = 0; i < deferred_renderer.m_output_attachment.size(); i++)
	{
		m_DeferredRendererOutputTextureId[i] = ++tex_id;
		m_Textures.push_back(deferred_renderer.m_output_attachment[i]);
	}

	for (size_t i = 0; i < model_renderer.m_Albedo_Output.size(); i++)
	{
		m_ModelRendererColorTextureId[i] = ++tex_id;
		m_Textures.push_back(model_renderer.m_Albedo_Output[i]);
	}

	for (size_t i = 0; i < model_renderer.m_Normal_Output.size(); i++)
	{
		m_ModelRendererNormalTextureId[i] = ++tex_id;
		m_Textures.push_back(model_renderer.m_Normal_Output[i]);
	}

	for (size_t i = 0; i < model_renderer.m_Depth_Output.size(); i++)
	{
		m_ModelRendererDepthTextureId[i] = ++tex_id;
		m_Textures.push_back(model_renderer.m_Depth_Output[i]);
	}

	// Shaders
	m_Shader.reset(new VulkanShader("ImGui.vert.spv", "ImGui.frag.spv"));

	if (!CreatePipeline(context.device))
	{
		LOG_ERROR("Failed to create pipeline for ImGui renderer.");
		assert(false);
	}

	/* Dynamic renderpass setup */
	UpdateAttachments();
}

void VulkanImGuiRenderer::UpdateAttachments()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_dyn_renderpass[i].color_attachments.clear();
		m_dyn_renderpass[i].has_depth_attachment = false;
		m_dyn_renderpass[i].has_stencil_attachment = false;

		m_dyn_renderpass[i].add_color_attachment(context.swapchain->color_attachments[i].view);
	}

}

void VulkanImGuiRenderer::render(size_t currentImageIdx, VkCommandBuffer cmdBuffer) 
{
	VULKAN_RENDER_DEBUG_MARKER(cmdBuffer, "ImGui");

	VkClearValue clearValue = { { 0.1f, 0.1f, 1.0f, 1.0f } };
	
	const uint32_t width  = context.swapchain->info.extent.width;
	const uint32_t height = context.swapchain->info.extent.height;
	VkRect2D renderArea{ .offset {0, 0}, .extent { width, height } };

	// Set viewport/scissor dynamically
	SetViewportScissor(cmdBuffer, width, height);

	m_dyn_renderpass[currentImageIdx].begin(cmdBuffer, renderArea);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfx_pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

	draw_scene(cmdBuffer);

	m_dyn_renderpass[currentImageIdx].end(cmdBuffer);
}


void VulkanImGuiRenderer::create_buffers()
{
	create_storage_buffer(m_storage_buffer, imgui_buffer_size);
	create_uniform_buffer(m_uniform_buffer, 1024);
}

void VulkanImGuiRenderer::draw_scene(VkCommandBuffer cmd_buffer)
{
	ImVec2 clipOff = m_pDrawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clipScale = m_pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	const uint32_t width = context.swapchain->info.extent.width;
	const uint32_t height = context.swapchain->info.extent.height;


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
				vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

				// Texture ID push constant
				if (m_Textures.size() > 0)
				{
					uint32_t texture = (uint32_t)(intptr_t)pcmd->TextureId;
					vkCmdPushConstants(cmd_buffer, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &pcmd->TextureId);
				}

				vkCmdDraw(cmd_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset);
			}
		}
		idxOffset += cmdList->IdxBuffer.Size;
		vtxOffset += cmdList->VtxBuffer.Size;
	}
}

void VulkanImGuiRenderer::update_buffers(ImDrawData* pDrawData)
{
	m_pDrawData = pDrawData;

	const float L = m_pDrawData->DisplayPos.x;
	const float R = m_pDrawData->DisplayPos.x + m_pDrawData->DisplaySize.x;
	const float T = m_pDrawData->DisplayPos.y;
	const float B = m_pDrawData->DisplayPos.y + m_pDrawData->DisplaySize.y;

	// UBO
	const glm::mat4 inMtx = glm::ortho(L, R, T, B);
	UploadBufferData(m_uniform_buffer, glm::value_ptr(inMtx), sizeof(glm::mat4), 0);
	
	// SBO : Vertex/Index data
	void* sboData = nullptr;
	vkMapMemory(context.device, m_storage_buffer.memory, 0, imgui_buffer_size, 0, &sboData);
	
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

	vkUnmapMemory(context.device, m_storage_buffer.memory);
}

VulkanImGuiRenderer::~VulkanImGuiRenderer()
{

}

bool VulkanImGuiRenderer::CreateFontTexture(ImGuiIO* io, const char* fontPath, Texture2D& out_Font)
{
	ImFontConfig cfg = ImFontConfig();
	cfg.FontDataOwnedByAtlas = false;
	cfg.SizePixels = 16.0f;
	
	ImFont* Font = io->Fonts->AddFontFromFileTTF(fontPath, cfg.SizePixels, &cfg);
	
	unsigned char* im_data = nullptr;
	int tex_width = 1, tex_height = 1, bpp = 0;
	io->Fonts->GetTexDataAsRGBA32(&im_data, &tex_width, &tex_height, &bpp);
	if (!im_data)
	{
		LOG_ERROR("Failed to load texture\n"); 
		return false;
	}
	out_Font.init(VK_FORMAT_R8G8B8A8_UNORM, tex_width, tex_height, false);
	out_Font.create_from_data(im_data, "Font texture", VK_IMAGE_USAGE_SAMPLED_BIT);
	out_Font.create_view(context.device, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
	io->FontDefault = Font;

	return true;
}

bool VulkanImGuiRenderer::CreatePipeline(VkDevice device)
{
	create_buffers();

	m_descriptor_pool = create_descriptor_pool(numStorageBuffers, numUniformBuffers, m_Textures.size());

	{
		std::vector<VkDescriptorSetLayoutBinding> bindings =
		{
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT },
			{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = (uint32_t)(m_Textures.size()), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		m_descriptor_set_layout = create_descriptor_set_layout(bindings);
	}

	m_descriptor_set = create_descriptor_set(m_descriptor_pool, m_descriptor_set_layout);
	update_descriptor_set(device);

	size_t fragConstRangeSizeInBytes = sizeof(uint32_t); // Size of 1 Push constant holding the texture ID passed from ImGui::Image
	m_pipeline_layout = create_pipeline_layout(device, m_descriptor_set_layout, 0, fragConstRangeSizeInBytes);

	GraphicsPipeline::Flags pp_flags = GraphicsPipeline::Flags::NONE;
	std::array<VkFormat, 1> color_formats = { ENGINE_SWAPCHAIN_COLOR_FORMAT };
	GraphicsPipeline::CreateDynamic(*m_Shader.get(), color_formats, {}, pp_flags, m_pipeline_layout, &m_gfx_pipeline, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	return true;
}

void VulkanImGuiRenderer::update_descriptor_set(VkDevice device)
{
	// Create descriptors for each texture used in ImGui 
	std::vector<VkDescriptorImageInfo> textureDescriptors;
	for (size_t i = 0; i < m_Textures.size(); i++)
	{
		m_Textures[i].descriptor = { .sampler = VulkanRendererBase::s_SamplerClampNearest, .imageView = m_Textures[i].view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		textureDescriptors.push_back(m_Textures[i].descriptor);
	}

	VkDescriptorBufferInfo uboInfo0{ .buffer = m_uniform_buffer.buffer, .offset = 0, .range = sizeof(glm::mat4) };
	VkDescriptorBufferInfo sboInfo0{ .buffer = m_storage_buffer.buffer, .offset = 0, .range = ImGuiVtxBufferSize };
	VkDescriptorBufferInfo sboInfo1{ .buffer = m_storage_buffer.buffer, .offset = ImGuiVtxBufferSize, .range = ImGuiIdxBufferSize };

	std::array<VkWriteDescriptorSet, 4> descriptorWrites
	{
		BufferWriteDescriptorSet(m_descriptor_set, 0, &uboInfo0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
		BufferWriteDescriptorSet(m_descriptor_set, 1, &sboInfo0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		BufferWriteDescriptorSet(m_descriptor_set, 2, &sboInfo1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		// Descriptor array
		VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_descriptor_set,
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(m_Textures.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = textureDescriptors.data()
		}
	};

	vkUpdateDescriptorSets(device, (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}
