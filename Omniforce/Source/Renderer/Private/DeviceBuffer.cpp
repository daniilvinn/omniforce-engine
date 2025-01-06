#include <Foundation/Common.h>
#include <Renderer/DeviceBuffer.h>

#include <Platform/Vulkan/VulkanDeviceBuffer.h>

namespace Omni {

	Ref<DeviceBuffer> DeviceBuffer::Create(IAllocator* allocator, const DeviceBufferSpecification& spec)
	{
		return CreateRef<VulkanDeviceBuffer>(allocator, spec);
	}

	Ref<DeviceBuffer> DeviceBuffer::Create(IAllocator* allocator, const DeviceBufferSpecification& spec, void* data, uint64 data_size)
	{
		return CreateRef<VulkanDeviceBuffer>(allocator, spec, data, data_size);
	}

}