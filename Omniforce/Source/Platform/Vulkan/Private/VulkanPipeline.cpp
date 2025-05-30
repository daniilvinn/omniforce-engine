#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanPipeline.h>

#include <Core/RuntimeExecutionContext.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanShader.h>
#include <Platform/Vulkan/VulkanImage.h>
#include <Platform/Vulkan/VulkanDescriptorSet.h>

namespace Omni {

#pragma region converts
	constexpr VkFormat convert(const DeviceDataType& type) {
		switch (type)
		{
		case DeviceDataType::INT:						return VK_FORMAT_R32_SINT;
		case DeviceDataType::INT2:						return VK_FORMAT_R32G32_SINT;
		case DeviceDataType::INT3:						return VK_FORMAT_R32G32B32_SINT;
		case DeviceDataType::INT4:						return VK_FORMAT_R32G32B32A32_SINT;
		case DeviceDataType::FLOAT:						return VK_FORMAT_R32_SFLOAT;
		case DeviceDataType::FLOAT2:					return VK_FORMAT_R32G32_SFLOAT;
		case DeviceDataType::FLOAT3:					return VK_FORMAT_R32G32B32_SFLOAT;
		case DeviceDataType::FLOAT4:					return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DeviceDataType::IMAT3:						return VK_FORMAT_R32G32B32_SINT;
		case DeviceDataType::IMAT4:						return VK_FORMAT_R32G32B32A32_SINT;
		case DeviceDataType::MAT3:						return VK_FORMAT_R32G32B32_SFLOAT;
		case DeviceDataType::MAT4:						return VK_FORMAT_R32G32B32A32_SFLOAT;
		default:										std::unreachable();
		}
	}

	constexpr VkPrimitiveTopology convert(const PipelineTopology& topology) {
		switch (topology)
		{
		case PipelineTopology::TRIANGLES:				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PipelineTopology::LINES:					return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PipelineTopology::POINTS:					return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		default:										std::unreachable();
		}
	}

	constexpr VkPolygonMode convert(const PipelineFillMode& mode) {
		switch (mode)
		{
		case PipelineFillMode::FILL:					return VK_POLYGON_MODE_FILL;
		case PipelineFillMode::EDGE_ONLY:				return VK_POLYGON_MODE_LINE;
		default:										std::unreachable();
		}
	}

	constexpr VkCullModeFlagBits convert(const PipelineCullingMode& mode) {
		switch (mode)
		{
		case PipelineCullingMode::BACK:					return VK_CULL_MODE_BACK_BIT;
		case PipelineCullingMode::FRONT:				return VK_CULL_MODE_FRONT_BIT;
		case PipelineCullingMode::NONE:					return VK_CULL_MODE_NONE;
		default:										std::unreachable();
		}
	}

	constexpr VkFrontFace convert(const PipelineFrontFace face) {
		switch (face)
		{
		case PipelineFrontFace::CLOCKWISE:				return VK_FRONT_FACE_CLOCKWISE;
		case PipelineFrontFace::COUNTER_CLOCKWISE:		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:										std::unreachable();
		}
	}

	constexpr VkCompareOp convert(const PipelineDepthTestOp op) {
		switch (op)
		{
		case PipelineDepthTestOp::LESS:					return VK_COMPARE_OP_LESS;
		case PipelineDepthTestOp::GREATER:				return VK_COMPARE_OP_GREATER;
		case PipelineDepthTestOp::GREATER_OR_EQUAL:		return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case PipelineDepthTestOp::EQUALS:				return VK_COMPARE_OP_EQUAL;
		default: std::unreachable();
		}
	}

#pragma endregion

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& spec)
		: m_Specification(spec)
	{
		switch (spec.type)
		{
		case PipelineType::GRAPHICS:		CreateGraphics(); break;
		case PipelineType::COMPUTE:			CreateCompute(); break;
		case PipelineType::RAY_TRACING:		CreateRayTracing(); break;
		default:							std::unreachable(); break;
		}

	}

	VulkanPipeline::~VulkanPipeline()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice()->Raw();
		auto pipeline = m_Pipeline;
		auto layout = m_PipelineLayout;

		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueObjectDeletion(
			[device, pipeline, layout]() mutable {
				vkDestroyPipelineLayout(device, layout, nullptr);
				vkDestroyPipeline(device, pipeline, nullptr);
			}
		);
	}

	void VulkanPipeline::CreateGraphics()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		// ==================
		// Vertex input state

		VkVertexInputBindingDescription vertex_input_binding = {};
		vertex_input_binding.binding = 0;
		vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertex_input_binding.stride = m_Specification.input_layout.GetStride();
		std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;

		VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		if (m_Specification.input_layout.GetStride()) {

			// Computing locations
			int previous_location_width = 0;
			for (const auto& element : m_Specification.input_layout.GetElements())
			{
				uint32_t location;
				float location_compute_result = (float)element.size / 16.0f;
				if (location_compute_result <= 1.0f)
				{
					location = previous_location_width;
					previous_location_width += 1;
				}
				else
				{
					location = previous_location_width;
					previous_location_width += (int32)location_compute_result;
				}

				vertex_input_attributes.push_back({
					.location = location,
					.binding = 0,
					.format = convert(element.format),
					.offset = element.offset
					});
			}

			vertex_input_state.vertexBindingDescriptionCount = 1;
			vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding;
			vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes.data();
			vertex_input_state.vertexAttributeDescriptionCount = (uint32)vertex_input_attributes.size();
		}
		else {
			vertex_input_state.vertexBindingDescriptionCount = 0;
			vertex_input_state.pVertexBindingDescriptions = nullptr;
			vertex_input_state.vertexAttributeDescriptionCount = 0;
			vertex_input_state.pVertexAttributeDescriptions = nullptr;
		}
		

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
		input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_state.topology = convert(m_Specification.topology);
		input_assembly_state.primitiveRestartEnable = m_Specification.primitive_restart_enable;

		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pDynamicStates = dynamic_states;
		dynamic_state.dynamicStateCount = 2;

		VkPipelineRasterizationStateCreateInfo rasterization_state = {};
		rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_state.depthClampEnable = VK_FALSE;
		rasterization_state.rasterizerDiscardEnable = VK_FALSE;
		rasterization_state.polygonMode = m_Specification.fill_mode == PipelineFillMode::FILL ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
		rasterization_state.lineWidth = m_Specification.line_width;
		rasterization_state.cullMode = convert(m_Specification.culling_mode);
		rasterization_state.frontFace = convert(m_Specification.front_face);

		fvec2 current_swapchain_extent = (fvec2)VulkanGraphicsContext::Get()->GetSwapchain()->GetSpecification().extent;
		VkViewport viewport = {};
		viewport = { 0, current_swapchain_extent.y, current_swapchain_extent.x, -current_swapchain_extent.y, 0.0f, 1.0f };

		VkRect2D scissor = {};
		scissor.extent = { (uint32)current_swapchain_extent.x, (uint32)current_swapchain_extent.y };
		scissor.offset = { 0,0 };

		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		std::vector<VkPipelineColorBlendAttachmentState> blend_states(m_Specification.output_attachments_formats.size());
		for (auto& state : blend_states) {
			state.blendEnable = m_Specification.color_blending_enable;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			state.colorWriteMask = VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
		}

		VkPipelineColorBlendStateCreateInfo color_blend_state = {};
		color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_state.attachmentCount = blend_states.size();
		color_blend_state.pAttachments = blend_states.data();
		color_blend_state.logicOpEnable = VK_FALSE;
		color_blend_state.logicOp = VK_LOGIC_OP_COPY;


		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
		depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_state.depthTestEnable = m_Specification.depth_test_enable;
		depth_stencil_state.depthWriteEnable = m_Specification.depth_write_enable;
		depth_stencil_state.depthCompareOp = convert(m_Specification.depth_test_op);
		depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_state.stencilTestEnable = VK_FALSE;

		uint32 supported_multisampling_flags = device->GetPhysicalDevice()->GetProperties().properties.limits.framebufferColorSampleCounts;

		if (!(supported_multisampling_flags & m_Specification.sample_count)) 
		{
			uint32 max_supported_multisampling = 1;
			for (uint32 i = (supported_multisampling_flags >> 1); i != 0; i >>= 1) 
				max_supported_multisampling *= 2;

			OMNIFORCE_CORE_WARNING("Requested to create pipeline \"{0}\" with multisample count \"{1}\". Setting maximum supported multisample count ({2})",
				m_Specification.debug_name, m_Specification.sample_count, max_supported_multisampling);

			m_Specification.sample_count = max_supported_multisampling;
		}

		VkPipelineMultisampleStateCreateInfo multisample_state = {};
		multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_state.sampleShadingEnable = m_Specification.multisampling_enable;
		multisample_state.rasterizationSamples = (VkSampleCountFlagBits)m_Specification.sample_count;

		std::vector<DescriptorBinding> global_bindings;
		global_bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
		global_bindings.push_back({ 1, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });
		global_bindings.push_back({ 2, DescriptorBindingType::ACCELERATION_STRUCTURE, 1, 0 });
		global_bindings.push_back({ 3, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });

		DescriptorSetSpecification dummy_descriptor_set_spec = {};
		dummy_descriptor_set_spec.bindings = std::move(global_bindings);

		Ref<DescriptorSet> dummy_set = DescriptorSet::Create(&g_TransientAllocator, dummy_descriptor_set_spec);
		WeakPtr<VulkanDescriptorSet> vk_dummy_set = dummy_set;

		VkPushConstantRange dummy_pc_range = {};
		dummy_pc_range.offset = 0;
		dummy_pc_range.size = 128;
		dummy_pc_range.stageFlags = VK_SHADER_STAGE_ALL;

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { vk_dummy_set->RawLayout() };
		std::vector<VkPushConstantRange> push_constant_ranges = { dummy_pc_range };

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = descriptor_set_layouts.size();
		pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.size();
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device->Raw(), &pipeline_layout_create_info, nullptr, &m_PipelineLayout));

		std::vector<VkFormat> formats;
		for (const auto& format : m_Specification.output_attachments_formats)
			formats.push_back(convert(format));

		VkPipelineRenderingCreateInfo pipeline_rendering = {};
		pipeline_rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipeline_rendering.colorAttachmentCount = formats.size();
		pipeline_rendering.pColorAttachmentFormats = formats.data();
		// HACK: I assume that D32_SFLOAT is chosen format. More proper way to do it is to request depth image format from pipeline spec or swapchain
		pipeline_rendering.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

		Ref<VulkanShader> vk_shader = m_Specification.shader;
		std::vector<VkPipelineShaderStageCreateInfo> stage_infos = vk_shader->GetCreateInfos();

		VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
		graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphics_pipeline_create_info.pNext = &pipeline_rendering;
		graphics_pipeline_create_info.pStages = stage_infos.data();
		graphics_pipeline_create_info.stageCount = stage_infos.size();
		graphics_pipeline_create_info.pVertexInputState = &vertex_input_state;
		graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state;
		graphics_pipeline_create_info.pDynamicState = &dynamic_state;
		graphics_pipeline_create_info.pRasterizationState = &rasterization_state;
		graphics_pipeline_create_info.pColorBlendState = &color_blend_state;
		graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state;
		graphics_pipeline_create_info.pMultisampleState = &multisample_state;
		graphics_pipeline_create_info.layout = m_PipelineLayout;
		graphics_pipeline_create_info.pViewportState = &viewport_state;
		graphics_pipeline_create_info.renderPass = VK_NULL_HANDLE;

		VkResult result = vkCreateGraphicsPipelines(device->Raw(), VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &m_Pipeline);
		if(result != VK_SUCCESS)
		{
			OMNIFORCE_CORE_ERROR("Failed to create pipeline \"{0}\".", m_Specification.debug_name);
			return;
		}

		OMNIFORCE_CORE_TRACE("Created pipeline \"{0}\"", m_Specification.debug_name);

	}

	void VulkanPipeline::CreateCompute()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		std::vector<DescriptorBinding> global_bindings;
		global_bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
		global_bindings.push_back({ 1, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });
		global_bindings.push_back({ 2, DescriptorBindingType::ACCELERATION_STRUCTURE, 1, 0 });
		global_bindings.push_back({ 3, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });

		DescriptorSetSpecification dummy_descriptor_set_spec = {};
		dummy_descriptor_set_spec.bindings = std::move(global_bindings);

		Ref<DescriptorSet> dummy_set = DescriptorSet::Create(&g_TransientAllocator, dummy_descriptor_set_spec);
		WeakPtr<VulkanDescriptorSet> vk_dummy_set = dummy_set;

		VkPushConstantRange dummy_pc_range = {};
		dummy_pc_range.offset = 0;
		dummy_pc_range.size = 128;
		dummy_pc_range.stageFlags = VK_SHADER_STAGE_ALL;

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts = { vk_dummy_set->RawLayout() };
		std::vector<VkPushConstantRange> push_constant_ranges = { dummy_pc_range };

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = descriptor_set_layouts.size();
		pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.size();
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device->Raw(), &pipeline_layout_create_info, nullptr, &m_PipelineLayout));

		Ref<VulkanShader> vk_shader = m_Specification.shader;
		std::vector<VkPipelineShaderStageCreateInfo> stage_create_info = vk_shader->GetCreateInfos();

		VkComputePipelineCreateInfo compute_pipeline_create_info = {};
		compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		compute_pipeline_create_info.layout = m_PipelineLayout;
		compute_pipeline_create_info.stage = stage_create_info[0];

		VkResult result = vkCreateComputePipelines(device->Raw(), VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			OMNIFORCE_CORE_ERROR("Failed to create pipeline \"{0}\".", m_Specification.debug_name);
			return;
		}

		OMNIFORCE_CORE_TRACE("Pipeline \"{0}\" created successfully", m_Specification.debug_name);

	}

	void VulkanPipeline::CreateRayTracing()
	{
		OMNIFORCE_ASSERT_TAGGED(false, "Use RTPipeline!");
	}

}