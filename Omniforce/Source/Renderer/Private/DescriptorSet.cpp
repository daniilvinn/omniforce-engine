#include <Foundation/Common.h>
#include <Renderer/DescriptorSet.h>

#include <Platform/Vulkan/VulkanDescriptorSet.h>

namespace Omni {

	Ref<DescriptorSet> DescriptorSet::Create(IAllocator* allocator, const DescriptorSetSpecification& spec)
	{
		return CreateRef<VulkanDescriptorSet>(allocator, spec);
	}

}