#pragma once

#include "../../Platform.h"
#include "../../BasicTypes.h"
#include "../../Log/Logger.h"
#include "../../Array.h"
#include "../Allocator.h"
#include "DedicatedMemoryAllocator.h"

#include <shared_mutex>
#include <limits>

namespace Omni {

	/*
	*  @brief Allocator that is used for data that may persistent in the memory across frames.
	*  Use with caution - may lead to up to x2 memory waste due to alignment requirement being a power of two.
	*/
	class OMNIFORCE_API PersistentAllocator : public IAllocator {
	public:

		PersistentAllocator(SizeType total_size, SizeType minimal_block_size = 4)
			: m_Size(Align(total_size))
			, m_MinimalBlockSize(minimal_block_size)
			, m_MaxSubdivisionLevel(std::log2(m_Size / m_MinimalBlockSize))
			, m_MemoryPool(&g_DedicatedMemoryAllocator, total_size)
		{
			// Init free lists
			m_FreeLists.resize(m_MaxSubdivisionLevel + 1);

			// Entire list is free
			m_FreeLists[m_MaxSubdivisionLevel].push_back(0); 
		}

		~PersistentAllocator() {
			
		}

		MemoryAllocation AllocateBase(SizeType size) override {
			std::lock_guard lock(m_Mutex);

			if (size == 0) return MemoryAllocation::InvalidAllocation();

			SizeType aligned_size = std::max(m_MinimalBlockSize, Align(size));
			SizeType level = std::log2(aligned_size / m_MinimalBlockSize);

			// Try find block in general pool
			for (SizeType i = level; i <= m_MaxSubdivisionLevel; ++i) {
				if (!m_FreeLists[i].empty()) {
					SizeType block = m_FreeLists[i].back();
					m_FreeLists[i].pop_back();

					// Subdivide the block if it is bigger than needed
					while (i > level) {
						--i;
						SizeType buddy = block + (SizeType(1) << i);
						m_FreeLists[i].push_back(buddy);
					}

					m_AllocatedMemoryBlocks[block] = aligned_size;

					return MemoryAllocation(&m_MemoryPool[block * m_MinimalBlockSize], aligned_size);;
				}
			}

			OMNIFORCE_CORE_CRITICAL("Persistent allocator: out of memory");

			// Out of memory
			return MemoryAllocation::InvalidAllocation();
		}

		void FreeBase(MemoryAllocation& allocation) override {
			if (!allocation.IsValid()) {
				OMNIFORCE_CORE_WARNING("Persistent allocator: attempted to free invalid block");
			}

			std::lock_guard lock(m_Mutex);

			SizeType block = (SizeType)allocation.Memory / m_MinimalBlockSize;
			auto it = m_AllocatedMemoryBlocks.find(block);

			OMNIFORCE_ASSERT_TAGGED(it != m_AllocatedMemoryBlocks.end(), "Invalid deallocation");

			SizeType block_size = it->second;
			SizeType level = log2(block_size / m_MinimalBlockSize);

			m_AllocatedMemoryBlocks.erase(it);

			// Try to merge with buddy
			while (level < m_MaxSubdivisionLevel) {
				SizeType buddy = block ^ (1 << level);

				auto it = std::find(m_FreeLists[level].begin(), m_FreeLists[level].end(), buddy);

				if (it != m_FreeLists[level].end()) {
					m_FreeLists[level].erase(it);
					block = std::min(block, buddy);

					++level;
				}
				else {
					break;
				}
			}

			m_FreeLists[level].push_back(block);

			allocation.Invalidate();
		}

		void Clear() override {
			std::lock_guard lock(m_Mutex);

			// Clear all free lists and reinitialize them
			for (auto& free_list : m_FreeLists) {
				free_list.clear();
			}
			m_FreeLists[m_MaxSubdivisionLevel].push_back(0); // Mark the entire memory pool as free

			// Clear allocated blocks map
			m_AllocatedMemoryBlocks.clear();

			OMNIFORCE_CORE_INFO("PersistentAllocator has been cleared and reset to its initial state.");
		}

		SizeType ComputeAlignedSize(SizeType size) override { 
			return Align(size); 
		};

	private:
		SizeType Align(SizeType n) const {
			return std::pow(2, std::ceil(std::log2(n)));
		}

		SizeType m_Size;
		SizeType m_MinimalBlockSize;
		SizeType m_MaxSubdivisionLevel;

		ByteArray m_MemoryPool;
		std::vector<std::vector<SizeType>> m_FreeLists;			// General free blocks
		std::map<SizeType, SizeType> m_AllocatedMemoryBlocks;	// Map of allocated blocks (address -> size)

		mutable std::shared_mutex m_Mutex;						// Mutex for concurrent access
	};

	/*
	*  @brief Managed allocator instance that is used for data that may persistent in the memory across frames.
	*  Use with caution - may lead to up to x2 memory waste due to alignment requirement being a power of two.
	*/
	inline PersistentAllocator g_PersistentAllocator = IAllocator::Setup<PersistentAllocator>(1024 * 1024 * 10);

}