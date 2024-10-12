#include "../Pipeline.h"

#include <Platform/Vulkan/VulkanPipeline.h>
#include "Scene/PipelineLibrary.h"

namespace Omni {

	Shared<Pipeline> Pipeline::Create(const PipelineSpecification& spec, UUID id)
	{
		auto pipeline = std::make_shared<VulkanPipeline>(spec);
		pipeline->m_ID = id;

		PipelineLibrary::AddPipeline(pipeline);

		return pipeline;
	}

}