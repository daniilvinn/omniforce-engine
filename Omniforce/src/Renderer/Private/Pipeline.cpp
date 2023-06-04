#include "../Pipeline.h"

#include <Platform/Vulkan/VulkanPipeline.h>

namespace Omni {

	Shared<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		return std::make_shared<VulkanPipeline>(spec);
	}

}