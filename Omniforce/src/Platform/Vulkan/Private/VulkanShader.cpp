#include "../VulkanShader.h"
#include "../VulkanGraphicsContext.h"

#include <Renderer/ShaderLibrary.h>

#include <spirv_reflect.h>

namespace Omni {

	VkShaderStageFlagBits convert(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT:		return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::COMPUTE:		return VK_SHADER_STAGE_COMPUTE_BIT;
		default:						std::unreachable();
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

	VulkanShader::VulkanShader(const ShaderStage& stage, const std::vector<uint32>& code, std::filesystem::path path)
		: m_Path(path)
	{
		ShaderLibrary* sl = ShaderLibrary::Get();
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		// Declare pipeline stage create info
		VkPipelineShaderStageCreateInfo stage_create_info = {};
		stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info.pName = "main";
		stage_create_info.stage = convert(stage);

		// Create a module for pipeline stage
		VkShaderModuleCreateInfo module_create_info = {};
		module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_create_info.pCode = code.data();
		module_create_info.codeSize = code.size() * 4;

		vkCreateShaderModule(device->Raw(), &module_create_info, nullptr, &stage_create_info.module);

		m_CreateInfo = stage_create_info;

		SpvReflectShaderModule spv_module;
		SpvReflectResult result = spvReflectCreateShaderModule(code.size() * 4, code.data(), &spv_module);

		uint32 descriptor_binding_count = 0;
		spvReflectEnumerateDescriptorBindings(&spv_module, &descriptor_binding_count, nullptr);

		std::vector<SpvReflectDescriptorSet*> descriptor_sets(descriptor_binding_count);
		spvReflectEnumerateDescriptorSets(&spv_module, &descriptor_binding_count, descriptor_sets.data());

		std::vector<VkDescriptorSetLayoutCreateInfo> descriptor_set_layout_create_infos;

		for (auto& set : descriptor_sets)
		{
			VkDescriptorSetLayoutCreateInfo layout_create_info = {};
			layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layout_create_info.bindingCount = set->binding_count;
			
			VkDescriptorSetLayoutBinding* bindings = new VkDescriptorSetLayoutBinding[set->binding_count];

			for (int i = 0; i < set->binding_count; i++) {

				VkDescriptorSetLayoutBinding layout_binding = {};
				layout_binding.binding = set->bindings[i]->binding;
				layout_binding.descriptorCount = set->bindings[i]->count;
				layout_binding.descriptorType = convert(set->bindings[i]->descriptor_type);
				layout_binding.stageFlags = VK_SHADER_STAGE_ALL;

				bindings[i] = layout_binding;
			}

			layout_create_info.pBindings = bindings;

			descriptor_set_layout_create_infos.push_back(layout_create_info);
		}

		std::vector<VkDescriptorSetLayout> set_layouts(descriptor_set_layout_create_infos.size());

		for (int i = 0; i < set_layouts.size(); i++) {
			vkCreateDescriptorSetLayout(device->Raw(), &descriptor_set_layout_create_infos[i], nullptr, &set_layouts[i]);
			delete[] descriptor_set_layout_create_infos[i].pBindings;
		}

		m_Layouts = std::move(set_layouts);

		// Reflect Push Constants
		uint32_t push_constant_count;
		spvReflectEnumeratePushConstants(&spv_module, &push_constant_count, nullptr);
		std::vector<SpvReflectBlockVariable*> ranges(push_constant_count);
		spvReflectEnumeratePushConstants(&spv_module, &push_constant_count, ranges.data());

		for (const auto& range : ranges) {
			VkPushConstantRange push_constant = {};
			push_constant.stageFlags = VK_SHADER_STAGE_ALL;
			push_constant.size = range->size;
			push_constant.offset = range->offset;

			m_Ranges.push_back(push_constant);
		}

		spvReflectDestroyShaderModule(&spv_module);
	}

	VulkanShader::~VulkanShader()
	{
		this->Destroy();
	}

	void VulkanShader::Destroy()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		if (m_CreateInfo.module != VK_NULL_HANDLE)
			vkDestroyShaderModule(device->Raw(), m_CreateInfo.module, nullptr);

		for (auto& layout : m_Layouts)
			vkDestroyDescriptorSetLayout(device->Raw(), layout, nullptr);

		m_Layouts.clear();
		m_CreateInfo.module = VK_NULL_HANDLE;
	}

	void VulkanShader::RestoreShaderModule(std::filesystem::path path)
	{
		OMNIFORCE_ASSERT_TAGGED(false, "Method not implemented");
	}

}