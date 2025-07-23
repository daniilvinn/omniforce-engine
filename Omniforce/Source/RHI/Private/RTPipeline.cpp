#include <Foundation/Common.h>
#include <RHI/RTPipeline.h>

#include <RHI/DescriptorSet.h>
#include <Platform/Vulkan/VulkanRTPipeline.h>

namespace Omni {

	Ref<RTPipeline> RTPipeline::Create(IAllocator* allocator, const RTPipelineSpecification& spec)
	{
		return CreateRef<VulkanRTPipeline>(allocator, spec);
	}

}