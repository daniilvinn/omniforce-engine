#pragma once

#include <Renderer/Shader.h>

#include <filesystem>
#include <vulkan/vulkan.h>

namespace Omni {

	class VulkanShader : public Shader {
	public:
		VulkanShader(const ShaderStage& stage, const std::vector<uint32>& code, std::filesystem::path path);
		~VulkanShader();

		VkPipelineShaderStageCreateInfo GetCreateInfo() const { return m_CreateInfo; }
		std::vector<VkDescriptorSetLayout> GetLayouts() const { return m_Layouts; }
		std::vector<VkPushConstantRange> GetRanges() const { return m_Ranges; }
		void Destroy() override;

		/*
		*	@brief returns status of "dirty" bit. It indicates if shader is still valid. It may be invalid due to several reasons:
		*	unsuccessful reflection, reload required etc.
		*/
		bool Dirty() const override { return m_Dirty; }
		void SetDirty(bool dirty) override { m_Dirty = dirty; }

		void RestoreShaderModule(std::filesystem::path path) override;

	private:
		VkPipelineShaderStageCreateInfo m_CreateInfo;
		std::vector<VkDescriptorSetLayout> m_Layouts;
		std::vector<VkPushConstantRange> m_Ranges;
		std::filesystem::path m_Path;
		bool m_Dirty = false;

	};

}