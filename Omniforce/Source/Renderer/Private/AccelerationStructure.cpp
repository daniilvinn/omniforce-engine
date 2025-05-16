#include <Foundation/Common.h>
#include <Renderer/AccelerationStructure.h>

#include <Platform/Vulkan/VulkanAccelerationStructure.h>

namespace Omni {

	Ptr<AccelerationStructure> AccelerationStructure::Create(IAllocator* allocator, const BLASBuildInfo& build_info)
	{
		return CreatePtr<VulkanAccelerationStructure>(allocator, build_info);
	}

	Ptr<AccelerationStructure> AccelerationStructure::Create(IAllocator* allocator, const TLASBuildInfo& build_info)
	{
		return CreatePtr<VulkanAccelerationStructure>(allocator, build_info);
	}

}