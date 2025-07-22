#include <Foundation/Common.h>
#include <Renderer/Pipeline.h>

#include <Platform/Vulkan/VulkanPipeline.h>
#include <Scene/PipelineLibrary.h>

namespace Omni {

	Ref<Pipeline> Pipeline::Create(IAllocator* allocator, const PipelineSpecification& spec, UUID id)
	{
		auto pipeline = CreateRef<VulkanPipeline>(allocator, spec);
		pipeline->m_ID = id;

		PipelineLibrary::AddPipeline(pipeline);

		return pipeline;
	}

}