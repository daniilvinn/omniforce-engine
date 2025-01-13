#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Log/Logger.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Containers/Array.h>
#include <Foundation/Memory/Allocators/DedicatedMemoryAllocator.h>

#include <shared_mutex>
#include <limits>

namespace Omni {

	class OMNIFORCE_API PersistentAllocator : public IAllocator {
	public:

		PersistentAllocator(TSize total_size, TSize minimal_block_size = 4)
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

		~PersistentAllocator() {}

		MemoryAllocation AllocateBase(TSize size) override {
			std::lock_guard lock(m_Mutex);

			if (size == 0) return MemoryAllocation::InvalidAllocation();

			TSize aligned_size = std::max(m_MinimalBlockSize, Align(size));
			TSize level = std::log2(aligned_size / m_MinimalBlockSize);

			// Try to find block in general pool
			for (TSize i = level; i <= m_MaxSubdivisionLevel; ++i) {
				if (!m_FreeLists[i].empty()) {
					TSize block = m_FreeLists[i].back();
					m_FreeLists[i].pop_back();

					// Subdivide the block if it is bigger than needed
					while (i > level) {
						--i;
						TSize buddy = block + (TSize(1) << i);
						m_FreeLists[i].push_back(buddy);
					}

					m_AllocatedMemoryBlocks[(byte*)&m_MemoryPool[block * m_MinimalBlockSize]] = aligned_size;

					// Allocate memory using MemoryAllocation structure
					return MemoryAllocation(
						&m_MemoryPool[block * m_MinimalBlockSize], aligned_size
					);
				}
			}

			OMNIFORCE_CORE_CRITICAL("Persistent allocator: out of memory");

			// Out of memory
			return MemoryAllocation::InvalidAllocation();
		}

		void FreeBase(MemoryAllocation& allocation) override {
			if (!allocation.IsValid()) {
				OMNIFORCE_CORE_WARNING("Persistent allocator: attempted to free invalid block");
				return;
			}

			std::lock_guard lock(m_Mutex);

			// Find block using address
			auto it = m_AllocatedMemoryBlocks.find(allocation.Memory);
			OMNIFORCE_ASSERT_TAGGED(it != m_AllocatedMemoryBlocks.end(), "Persistent allocator: attempted to free invalid or untracked block");

			TSize block_size = it->second;
			TSize block = (TSize)(allocation.Memory - (byte*)&m_MemoryPool[0]) / m_MinimalBlockSize;
			TSize level = std::log2(block_size / m_MinimalBlockSize);

			m_AllocatedMemoryBlocks.erase(it);

			// Try to merge with buddy
			while (level < m_MaxSubdivisionLevel) {
				TSize buddy = block ^ (1 << level);

				auto buddy_it = std::find(m_FreeLists[level].begin(), m_FreeLists[level].end(), buddy);

				if (buddy_it != m_FreeLists[level].end()) {
					m_FreeLists[level].erase(buddy_it);
					block = std::min(block, buddy); // Merge the blocks
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

		TSize ComputeAlignedSize(TSize size) override {
			return Align(size);
		};

	private:
		TSize Align(TSize n) const {
			return std::pow(2, std::ceil(std::log2(n)));
		}

		TSize m_Size;
		TSize m_MinimalBlockSize;
		TSize m_MaxSubdivisionLevel;

		ByteArray m_MemoryPool; // Memory pool, used with MemoryAllocation
		std::vector<std::vector<TSize>> m_FreeLists; // Free block lists by level
		std::map<byte*, TSize> m_AllocatedMemoryBlocks; // Map of allocated blocks (address -> size)

		mutable std::shared_mutex m_Mutex; // Mutex for thread safety
	};

	inline PersistentAllocator g_PersistentAllocator = IAllocator::Setup<PersistentAllocator>(1024 * 1024 * 256);

}
