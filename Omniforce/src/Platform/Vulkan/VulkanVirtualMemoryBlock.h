#pragma once

#include <Memory/VirtualMemoryBlock.h>

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

	class VulkanVirtualMemoryBlock : public VirtualMemoryBlock {
	public:
		VulkanVirtualMemoryBlock(uint32 size);

		void Destroy() override;
		void Clear() override;
		uint32 Allocate(uint32 size, uint32 alignment = 0) override;
		void Free(uint32 offset) override;

		uint32 GetUsedMemorySize() override;
		uint32 GetFreeMemorySize() override;

	private:
		uint64 m_Size;
		VmaVirtualBlock m_MemoryBlock;
		rh::unordered_map<uint32, VmaVirtualAllocation> m_Map; // offset - allocation map

	};

}