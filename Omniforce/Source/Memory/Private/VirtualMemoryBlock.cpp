#include "../VirtualMemoryBlock.h"

#include <Platform/Vulkan/VulkanVirtualMemoryBlock.h>

#include <memory>

namespace Omni {

	Scope<VirtualMemoryBlock> VirtualMemoryBlock::Create(uint32 size)
	{
		return std::make_unique<VulkanVirtualMemoryBlock>(size);
	}

}