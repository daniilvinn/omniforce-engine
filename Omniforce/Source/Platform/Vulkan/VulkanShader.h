#pragma once

#include <Foundation/Common.h>

#include <RHI/Shader.h>

#include <map>

#include <volk.h>

namespace Omni {

	class VulkanShader : public Shader {
	public:
		VulkanShader(std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path);
		~VulkanShader();

		std::vector<VkPipelineShaderStageCreateInfo> GetCreateInfos() const { return m_StageCreateInfos; }
		std::vector<VkDescriptorSetLayout> GetLayouts() const { return m_SetLayouts; }
		std::vector<VkPushConstantRange> GetRanges() const { return m_Ranges; }
		void Destroy() override;

		/*
		*	@brief returns status of "dirty" bit. It indicates if shader is still valid. It may be invalid due to several reasons:
		*	unsuccessful reflection, reload required etc.
		*/

		void RestoreShaderModule(std::filesystem::path path) override;

	private:
		std::vector<VkPipelineShaderStageCreateInfo> m_StageCreateInfos;
		std::vector<VkDescriptorSetLayout> m_SetLayouts;
		std::vector<VkPushConstantRange> m_Ranges;
		std::filesystem::path m_Path;
		bool m_Dirty = false;

	};

}