#pragma once

#include <Renderer/Pipeline.h>

#include "VulkanCommon.h"

namespace Omni {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineSpecification& spec);
		~VulkanPipeline();

		void Destroy() override;

		VkPipeline Raw() const { return m_Pipeline; }
		VkPipelineLayout RawLayout() const { return m_PipelineLayout; }

	private:
		void CreateGraphics();
		void CreateCompute();

	private:
		PipelineSpecification m_Specification;

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

	};

}