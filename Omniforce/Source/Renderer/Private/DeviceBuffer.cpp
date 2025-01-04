#include <Foundation/Common.h>
#include <Renderer/DeviceBuffer.h>

#include <Platform/Vulkan/VulkanDeviceBuffer.h>

namespace Omni {

	Shared<DeviceBuffer> DeviceBuffer::Create(const DeviceBufferSpecification& spec)
	{
		return std::make_shared<VulkanDeviceBuffer>(spec);
	}

	Shared<DeviceBuffer> DeviceBuffer::Create(const DeviceBufferSpecification& spec, void* data, uint64 data_size)
	{
		return std::make_shared<VulkanDeviceBuffer>(spec, data, data_size);
	}

}