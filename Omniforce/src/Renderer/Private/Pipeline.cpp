#include "../Pipeline.h"

#include <Platform/Vulkan/VulkanPipeline.h>
#include "Scene/PipelineLibrary.h"

namespace Omni {

	Shared<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		auto pipeline = std::make_shared<VulkanPipeline>(spec);
		PipelineLibrary::AddPipeline(pipeline);
		return pipeline;
	}

}