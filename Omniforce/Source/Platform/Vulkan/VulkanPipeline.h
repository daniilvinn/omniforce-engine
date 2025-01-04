#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Renderer/Pipeline.h>

namespace Omni {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineSpecification& spec);
		~VulkanPipeline();

		void Destroy() override;

		VkPipeline Raw() const { return m_Pipeline; }
		VkPipelineLayout RawLayout() const { return m_PipelineLayout; }
		const PipelineSpecification& GetSpecification() const override { return m_Specification; }

	private:
		void CreateGraphics();
		void CreateCompute();

	private:
		PipelineSpecification m_Specification;

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

	};

}