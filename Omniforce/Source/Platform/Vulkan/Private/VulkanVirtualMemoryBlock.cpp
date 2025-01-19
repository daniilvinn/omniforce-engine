#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanVirtualMemoryBlock.h>

#include <Core/Utils.h>

namespace Omni {

	VulkanVirtualMemoryBlock::VulkanVirtualMemoryBlock(uint32 size)
	{
		VmaVirtualBlockCreateInfo virtual_block_create_info = {};
		virtual_block_create_info.size = size;
		
		vmaCreateVirtualBlock(&virtual_block_create_info, &m_MemoryBlock);
	}

	void VulkanVirtualMemoryBlock::Clear()
	{
		vmaClearVirtualBlock(m_MemoryBlock);
	}

	void VulkanVirtualMemoryBlock::Destroy()
	{
		vmaClearVirtualBlock(m_MemoryBlock);
		vmaDestroyVirtualBlock(m_MemoryBlock);
	}

	uint32 VulkanVirtualMemoryBlock::Allocate(uint32 size, uint32 alignment)
	{
		OMNIFORCE_ASSERT_TAGGED(Utils::IsPowerOf2(alignment) || (alignment == 1), "Alignment virtual allocation must be power of two");

		VmaVirtualAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.size = size;
		allocation_create_info.alignment = alignment;

		VmaVirtualAllocation virtual_allocation;
		uint64 offset;
		vmaVirtualAllocate(m_MemoryBlock, &allocation_create_info, &virtual_allocation, &offset);

		m_Map.emplace(offset, virtual_allocation);

		return offset;
	}

	void VulkanVirtualMemoryBlock::Free(uint32 offset)
	{
		vmaVirtualFree(m_MemoryBlock, m_Map.at(offset));
		m_Map.erase(offset);
	}

	uint32 VulkanVirtualMemoryBlock::GetUsedMemorySize()
	{
		VmaStatistics stats = {};
		vmaGetVirtualBlockStatistics(m_MemoryBlock, &stats);

		return stats.allocationBytes;
	}

	uint32 VulkanVirtualMemoryBlock::GetFreeMemorySize()
	{
		VmaStatistics stats = {};
		vmaGetVirtualBlockStatistics(m_MemoryBlock, &stats);

		return stats.blockBytes - stats.allocationBytes;
	}

}