#include <Foundation/Common.h>
#include <RHI/AccelerationStructure.h>

#include <Platform/Vulkan/VulkanAccelerationStructure.h>

namespace Omni {

	Ptr<RTAccelerationStructure> RTAccelerationStructure::Create(IAllocator* allocator, const BLASBuildInfo& build_info)
	{
		return CreatePtr<VulkanRTAccelerationStructure>(allocator, build_info);
	}

	Ptr<RTAccelerationStructure> RTAccelerationStructure::Create(IAllocator* allocator, const TLASBuildInfo& build_info)
	{
		return CreatePtr<VulkanRTAccelerationStructure>(allocator, build_info);
	}

}