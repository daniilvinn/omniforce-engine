#include <Foundation/Common.h>
#include <Renderer/RTPipeline.h>

#include <Platform/Vulkan/VulkanRTPipeline.h>

namespace Omni {

	Ref<RTPipeline> RTPipeline::Create(IAllocator* allocator, const RTPipelineSpecification& spec)
	{
		return CreateRef<VulkanRTPipeline>(allocator, spec);
	}

	const RTPipelineSpecification& RTPipeline::GetSpecification() const
	{

	}

}