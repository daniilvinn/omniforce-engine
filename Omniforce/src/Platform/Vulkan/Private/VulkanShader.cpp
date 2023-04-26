#include "../VulkanShader.h"

#include <Platform/Vulkan/VulkanGraphicsContext.h>

#include <SPIRV-Reflect/spirv_reflect.h>

namespace Omni {

#pragma region converts 
	VkShaderStageFlagBits convert(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT:		return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::COMPUTE:		return VK_SHADER_STAGE_COMPUTE_BIT;
		}
	}

	VkDescriptorType convert(SpvReflectDescriptorType type) {
		switch (type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:							return VK_DESCRIPTOR_TYPE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:						return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:						return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:				return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:				return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:					return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:		return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		default:															std::unreachable();
		}
	}
#pragma endregion

	VulkanShader::VulkanShader(std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path)
		: m_Path(path)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		// set id and its bindings
		std::map<uint32, std::vector<VkDescriptorSetLayoutBinding>> bindings;

		for (auto& stage_data : binaries) {
			VkShaderModule shader_module;

			VkShaderModuleCreateInfo shader_module_create_info = {};
			shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_module_create_info.pCode = stage_data.second.data();
			shader_module_create_info.codeSize = stage_data.second.size() * 4;
			
			VK_CHECK_RESULT(vkCreateShaderModule(device->Raw(), &shader_module_create_info, nullptr, &shader_module));

			VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
			shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage_create_info.stage = convert(stage_data.first);
			shader_stage_create_info.pName = "main";
			shader_stage_create_info.module = shader_module;

			m_StageCreateInfos.push_back(shader_stage_create_info);

			SpvReflectShaderModule reflect_module;
			if (spvReflectCreateShaderModule(stage_data.second.size() * 4, stage_data.second.data(), &reflect_module) != SPV_REFLECT_RESULT_SUCCESS) {
				OMNIFORCE_CORE_ERROR("Failed to reflect shader {}", path.filename().string());
				m_Dirty = true;
				return;
			}

			uint32 set_count = 0;
			spvReflectEnumerateDescriptorSets(&reflect_module, &set_count, nullptr);

			std::vector<SpvReflectDescriptorSet*> reflect_descriptor_sets(set_count);
			spvReflectEnumerateDescriptorSets(&reflect_module, &set_count, reflect_descriptor_sets.data());

			for (auto& reflect_set : reflect_descriptor_sets) {
				if (bindings.find(reflect_set->set) != bindings.end()) {
					bindings.emplace(reflect_set->set, std::vector<VkDescriptorSetLayoutBinding>());
				}

				for (int i = 0; i < reflect_set->binding_count; i++) {
					SpvReflectDescriptorBinding* reflect_binding = reflect_set->bindings[i];

					VkDescriptorSetLayoutBinding layout_binding = {};
					layout_binding.binding = reflect_binding->binding;
					layout_binding.descriptorType = convert(reflect_binding->descriptor_type);
					layout_binding.descriptorCount = reflect_binding->count;
					layout_binding.stageFlags = VK_SHADER_STAGE_ALL;

					bool binding_already_registered = false;

					for (auto& set_binding : bindings[reflect_set->set]) {
						if (set_binding.binding == layout_binding.binding) {
							continue;
						}
					}
					
					bindings[reflect_set->set].push_back(layout_binding);
				}
			}

			uint32 push_constant_range_count = 0;
			spvReflectEnumeratePushConstants(&reflect_module, &push_constant_range_count, nullptr);

			std::vector<SpvReflectBlockVariable*> reflect_push_constant_ranges(push_constant_range_count);
			spvReflectEnumeratePushConstants(&reflect_module, &push_constant_range_count, reflect_push_constant_ranges.data());

			for (auto& reflect_range : reflect_push_constant_ranges) {
				for (auto& range : m_Ranges) {
					if (range.offset == reflect_range->offset && range.size == reflect_range->size) {
						range.stageFlags |= convert(stage_data.first);
						continue;
					}
				}

				VkPushConstantRange push_constant_range = {};
				push_constant_range.size = reflect_range->size;
				push_constant_range.offset = reflect_range->offset;
				push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;

				m_Ranges.push_back(push_constant_range);
			}

			spvReflectDestroyShaderModule(&reflect_module);
		}

		if (bindings.size()) {
			uint32 highest_set_index = bindings.rbegin()->first;
			for (int i = 0; i <= highest_set_index; i++) {
				if (bindings.find(i) == bindings.end())
					m_SetLayouts.push_back(VK_NULL_HANDLE);
				else {
					std::vector<VkDescriptorBindingFlags> binding_flags;
					for (auto& binding : bindings[i]) {
						if (binding.descriptorCount != 1) binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
						else binding_flags.push_back(0);
					}

					VkDescriptorSetLayoutBindingFlagsCreateInfo layout_bindings_flags = {};
					layout_bindings_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
					layout_bindings_flags.bindingCount = binding_flags.size();
					layout_bindings_flags.pBindingFlags = binding_flags.data();

					VkDescriptorSetLayoutCreateInfo layout_create_info = {};
					layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layout_create_info.pNext = &layout_bindings_flags;
					layout_create_info.bindingCount = bindings[i].size();
					layout_create_info.pBindings = bindings[i].data();

					VkDescriptorSetLayout vk_descriptor_set_layout = VK_NULL_HANDLE;
					vkCreateDescriptorSetLayout(device->Raw(), &layout_create_info, nullptr, &vk_descriptor_set_layout);
					m_SetLayouts.push_back(vk_descriptor_set_layout);
				}
			}
		}
	}

	VulkanShader::~VulkanShader()
	{
		
	}

	void VulkanShader::Destroy()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		for (auto& stage : m_StageCreateInfos)
			if (stage.module != VK_NULL_HANDLE)
				vkDestroyShaderModule(device->Raw(), stage.module, nullptr);

		for (auto& layout : m_SetLayouts)
			vkDestroyDescriptorSetLayout(device->Raw(), layout, nullptr);

		m_SetLayouts.clear();
		for (auto& stage : m_StageCreateInfos)
			stage.module = VK_NULL_HANDLE;
		
	}

	void VulkanShader::RestoreShaderModule(std::filesystem::path path)
	{

	}


}
