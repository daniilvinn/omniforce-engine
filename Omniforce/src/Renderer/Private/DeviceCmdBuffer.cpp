#include "../DeviceCmdBuffer.h"

#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>

namespace Omni {

	Shared<DeviceCmdBuffer> DeviceCmdBuffer::Create(DeviceCmdBufferLevel level, DeviceCmdBufferType buffer_type, DeviceCmdType cmd_type)
	{
		return std::make_shared<VulkanDeviceCmdBuffer>(level, buffer_type, cmd_type);
	}

}