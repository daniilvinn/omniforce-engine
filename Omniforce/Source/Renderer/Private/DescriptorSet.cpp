#include <Foundation/Common.h>
#include <Renderer/DescriptorSet.h>

#include <Platform/Vulkan/VulkanDescriptorSet.h>

namespace Omni {

	Shared<DescriptorSet> DescriptorSet::Create(const DescriptorSetSpecification& spec)
	{
		return std::make_shared<VulkanDescriptorSet>(spec);
	}

}