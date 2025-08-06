#include <Foundation/Common.h>
#include <RHI/Pipeline.h>

#include <Platform/Vulkan/VulkanPipeline.h>
#include <Rendering/PipelineLibrary.h>

namespace Omni {

	Ref<Pipeline> Pipeline::Create(IAllocator* allocator, const PipelineSpecification& spec, UUID id)
	{
		auto pipeline = CreateRef<VulkanPipeline>(allocator, spec);
		pipeline->m_ID = id;

		PipelineLibrary::AddPipeline(pipeline);

		return pipeline;
	}

}