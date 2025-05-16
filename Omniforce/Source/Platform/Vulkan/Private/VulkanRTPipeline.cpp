#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanRTPipeline.h>

#include <Platform/Vulkan/VulkanDescriptorSet.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanShader.h>

namespace Omni {

	VulkanRTPipeline::VulkanRTPipeline(const RTPipelineSpecification& spec)
		: RTPipeline(spec)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		// Create global descriptor set
		std::vector<DescriptorBinding> global_bindings;
		global_bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
		global_bindings.push_back({ 1, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });

		DescriptorSetSpecification dummy_descriptor_set_spec = {};
		dummy_descriptor_set_spec.bindings = std::move(global_bindings);

		Ref<DescriptorSet> dummy_set = DescriptorSet::Create(&g_TransientAllocator, dummy_descriptor_set_spec);
		WeakPtr<VulkanDescriptorSet> vk_dummy_set = dummy_set;

		// Create full push constant range
		VkPushConstantRange dummy_pc_range = {};
		dummy_pc_range.offset = 0;
		dummy_pc_range.size = 128;
		dummy_pc_range.stageFlags = VK_SHADER_STAGE_ALL;

		Array<VkDescriptorSetLayout> descriptor_set_layouts(&g_TransientAllocator);
		descriptor_set_layouts.Add(vk_dummy_set->RawLayout());
		Array<VkPushConstantRange> push_constant_ranges(&g_TransientAllocator);
		push_constant_ranges.Add(dummy_pc_range);

		// Create pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = descriptor_set_layouts.Size();
		pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.Raw();
		pipeline_layout_create_info.pushConstantRangeCount = push_constant_ranges.Size();
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.Raw();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device->Raw(), &pipeline_layout_create_info, nullptr, &m_PipelineLayout));

		// Create dynamic states
		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pDynamicStates = dynamic_states;
		dynamic_state.dynamicStateCount = 2;

		Array<VkPipelineShaderStageCreateInfo> all_stages(&g_TransientAllocator);
		Array<VkRayTracingShaderGroupCreateInfoKHR> groups = AssembleVulkanGroups(all_stages, m_Specification.groups);

		VkRayTracingPipelineCreateInfoKHR rt_pipeline_create_info = {};
		rt_pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rt_pipeline_create_info.maxPipelineRayRecursionDepth = spec.recursion_depth;
		rt_pipeline_create_info.pDynamicState = &dynamic_state;
		rt_pipeline_create_info.layout = m_PipelineLayout;
		rt_pipeline_create_info.pStages = all_stages.Raw();
		rt_pipeline_create_info.stageCount = all_stages.Size();
		rt_pipeline_create_info.pGroups = groups.Raw();
		rt_pipeline_create_info.groupCount = groups.Size();
		
		vkCreateRayTracingPipelinesKHR(device->Raw(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rt_pipeline_create_info, nullptr, &m_Pipeline);
	}

	VulkanRTPipeline::~VulkanRTPipeline()
	{

	}

	uint32 VulkanRTPipeline::AppendStage(Array<VkPipelineShaderStageCreateInfo>& all_stages, Ref<Shader> shader)
	{
		if (!shader) {
			return VK_SHADER_UNUSED_KHR;
		}

		WeakPtr<VulkanShader> vk_shader = shader;

		auto stages = vk_shader->GetCreateInfos();

		uint32 index = all_stages.Size();

		for (auto& stage : stages) {
			all_stages.Add(stage);
		}
		
		return index; 
	}

	Array<VkRayTracingShaderGroupCreateInfoKHR> VulkanRTPipeline::AssembleVulkanGroups(Array<VkPipelineShaderStageCreateInfo>& all_stages, const Array<ShaderGroup>& groups)
	{
		Array<VkRayTracingShaderGroupCreateInfoKHR> vk_shader_groups(&g_TransientAllocator);
		for (const ShaderGroup& group : groups)
		{
			if (group.ray_generation)
			{
				uint32 general_index = AppendStage(all_stages, group.ray_generation);

				VkRayTracingShaderGroupCreateInfoKHR group_info{};
				group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
				group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				group_info.generalShader = general_index;
				group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
				group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
				group_info.intersectionShader = VK_SHADER_UNUSED_KHR;

				vk_shader_groups.Add(group_info);
			}

			if (group.miss)
			{
				uint32 general_index = AppendStage(all_stages, group.miss);

				VkRayTracingShaderGroupCreateInfoKHR group_info{};
				group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
				group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				group_info.generalShader = general_index;
				group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
				group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
				group_info.intersectionShader = VK_SHADER_UNUSED_KHR;

				vk_shader_groups.Add(group_info);
			}

			// HIT GROUP
			if (group.closest_hit || group.any_hit || group.intersection)
			{
				VkRayTracingShaderGroupCreateInfoKHR group_info{};
				group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;

				bool has_intersection_shader = group.intersection;
				group_info.type = has_intersection_shader
					? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
					: VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

				group_info.generalShader = VK_SHADER_UNUSED_KHR;
				group_info.closestHitShader = AppendStage(all_stages, group.closest_hit);
				group_info.anyHitShader = AppendStage(all_stages, group.any_hit);
				group_info.intersectionShader = AppendStage(all_stages, group.intersection);

				vk_shader_groups.Add(group_info);
			}
		}

		return vk_shader_groups;
	}

}