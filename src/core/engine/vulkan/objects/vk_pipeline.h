#pragma once

namespace vk
{
	struct Pipeline
	{
		enum Flags
		{
			NONE = 1 << 0,
			ENABLE_ALPHA_BLENDING = 1 << 1,
			ENABLE_DEPTH_STATE = 1 << 2,
			DISABLE_VTX_INPUT_STATE = 1 << 3
		};

		void cmd_push_constants(VkCommandBuffer cmd_buffer, std::string_view push_constant_range_name, const void* p_values)
		{
			if (layout.ranges.contains(push_constant_range_name.data()))
			{
				VkPushConstantRange& range = layout.ranges.at(push_constant_range_name.data());
				vkCmdPushConstants(cmd_buffer, layout.vk_pipeline_layout, range.stageFlags, range.offset, range.size, p_values);
			}
			else
			{
				assert(false);
			}
		}

		struct Layout
		{
			operator VkPipelineLayout() { return vk_pipeline_layout; }

			void add_push_constant_range(const std::string& name, VkPushConstantRange range)
			{
				ranges[name] = range;
				push_constant_ranges.push_back(range);
			}

			void create(std::span<VkDescriptorSetLayout> set_layouts)
			{
				VkPipelineLayoutCreateInfo pipeline_layout_info = {};
				pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
				pipeline_layout_info.pSetLayouts = set_layouts.data();
				pipeline_layout_info.pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data();
				pipeline_layout_info.pushConstantRangeCount = (uint32_t)push_constant_ranges.size();

				vkCreatePipelineLayout(ctx.device, &pipeline_layout_info, nullptr, &vk_pipeline_layout);
			}

			std::unordered_map<std::string, VkPushConstantRange> ranges;
			std::vector<VkPushConstantRange> push_constant_ranges;

			VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;

		} layout;

		VkPipeline pipeline;

		operator VkPipeline() { return pipeline; }

		void create_graphics(const VertexFragmentShader& shader, std::span<VkFormat> color_formats, VkFormat depth_format, Flags flags, VkPipelineLayout pipeline_layout,
			VkPrimitiveTopology topology, VkCullModeFlags cull_mode, VkFrontFace front_face, uint32_t view_mask = 0, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
		void create_graphics(const VertexFragmentShader& shader, uint32_t numColorAttachments, Flags flags, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPrimitiveTopology topology,
			VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRenderingCreateInfoKHR* dynamic_pipeline_create,
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
		void create_compute(const Shader& shader);
		bool reload_pipeline();

		void bind(VkCommandBuffer cmd_buffer) const;

		bool is_graphics = false;

		/* Saved pipeline info */
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		VkPipelineRenderingCreateInfoKHR dynamic_pipeline_create_info = {};
		VertexFragmentShader pipeline_shader;
		std::vector<VkFormat> color_attachment_formats;
		VkFormat depth_attachment_format;
		Flags pipeline_flags;

		std::vector<VkDynamicState> pipeline_dynamic_states
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			//VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
		};
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};

		VkPipelineVertexInputStateCreateInfo vertexInputState;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
		VkPipelineMultisampleStateCreateInfo multisampleState;
		VkPipelineDepthStencilStateCreateInfo depthStencilState;
		VkPipelineColorBlendStateCreateInfo colorBlendState;
		VkPipelineDynamicStateCreateInfo dynamicState;
		VkPipelineViewportStateCreateInfo viewportState = {};
		VkPipelineRasterizationStateCreateInfo raster_state_info = {};
	};

	static Pipeline::Flags operator|(Pipeline::Flags a, Pipeline::Flags b)
	{
		return static_cast<Pipeline::Flags>(static_cast<int>(a) | static_cast<int>(b));
	}
}