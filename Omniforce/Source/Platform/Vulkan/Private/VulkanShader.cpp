#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanShader.h>

#include <Platform/Vulkan/VulkanGraphicsContext.h>

#include <SPIRV-Reflect/spirv_reflect.c>

namespace Omni {

#pragma region converts
	constexpr VkShaderStageFlagBits convert(const ShaderStage& stage) {
		switch (stage)
		{
		case ShaderStage::VERTEX:		return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::FRAGMENT:		return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::COMPUTE:		return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderStage::TASK:			return VK_SHADER_STAGE_TASK_BIT_EXT;
		case ShaderStage::MESH:			return VK_SHADER_STAGE_MESH_BIT_EXT;
		}

		return (VkShaderStageFlagBits)0;
	}

	constexpr VkDescriptorType convert(SpvReflectDescriptorType type) {
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
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}
	}

	constexpr std::string DescriptorToString(VkDescriptorType type) {
		switch (type)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:						return "sampler";
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:			return "combined image sampler";
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:					return "sampled image";
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:					return "storage image";
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:			return "uniform texel buffer";
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:			return "storage texel buffer";
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:					return "uniform buffer";
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:					return "storage buffer";
		default:												std::unreachable();
		}
	}
#pragma endregion

	VulkanShader::VulkanShader(std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path)
		: m_Path(path)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		// set id and its bindings
		std::map<uint32, std::vector<VkDescriptorSetLayoutBinding>> bindings;
		VkPushConstantRange push_constant_range = {};

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
