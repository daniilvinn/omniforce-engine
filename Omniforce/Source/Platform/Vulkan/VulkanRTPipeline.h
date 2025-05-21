#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Renderer/RTPipeline.h>
#include <Renderer/ShaderBindingTable.h>

namespace Omni {

	class VulkanRTPipeline : public RTPipeline {
	public:
		VulkanRTPipeline(const RTPipelineSpecification& spec);
		~VulkanRTPipeline();

		VkPipeline Raw() const { return m_Pipeline; }
		VkPipelineLayout RawLayout() const { return m_PipelineLayout; }
		const ShaderBindingTable& GetSBT() const override { return m_SBT; };

	private:
		uint32 AppendStage(Array<VkPipelineShaderStageCreateInfo>& stages, Ref<Shader> shader);
		Array<VkRayTracingShaderGroupCreateInfoKHR> AssembleVulkanGroups(Array<VkPipelineShaderStageCreateInfo>& all_stages, const Array<RTShaderGroup>& groups);

	private:
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
		ShaderBindingTable m_SBT;

	};

}