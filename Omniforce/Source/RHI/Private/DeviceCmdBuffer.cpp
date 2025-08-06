#include <Foundation/Common.h>
#include <RHI/DeviceCmdBuffer.h>

#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>

namespace Omni {

	Ref<DeviceCmdBuffer> DeviceCmdBuffer::Create(IAllocator* allocator, DeviceCmdBufferLevel level, DeviceCmdBufferType buffer_type, DeviceCmdType cmd_type)
	{
		return CreateRef<VulkanDeviceCmdBuffer>(allocator, level, buffer_type, cmd_type);
	}

}