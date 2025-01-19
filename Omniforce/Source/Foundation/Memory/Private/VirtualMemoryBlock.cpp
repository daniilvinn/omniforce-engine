#include "../VirtualMemoryBlock.h"

#include <Platform/Vulkan/VulkanVirtualMemoryBlock.h>
#include <Foundation/Memory/Allocators/PersistentAllocator.h>

#include <memory>

namespace Omni {

	Ptr<VirtualMemoryBlock> VirtualMemoryBlock::Create(IAllocator* allocator, uint32 size)
	{
		return CreatePtr<VulkanVirtualMemoryBlock>(allocator, size);
	}

}