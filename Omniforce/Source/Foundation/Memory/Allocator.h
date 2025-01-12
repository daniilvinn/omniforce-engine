#pragma once

#include <Foundation/Platform.h>

#include <Foundation/Memory/MemoryAllocation.h>

namespace Omni {

	class OMNIFORCE_API IAllocator {
	public:
		using TSize = uint64_t;

		struct Stats {
			TSize frame_memory_usage;
			TSize a;
		};

		virtual ~IAllocator() {};

		template<typename T, typename... TArgs>
		MemoryAllocation Allocate(TArgs&&... args) {
			MemoryAllocation Allocation = AllocateBase(sizeof(T));
			Allocation.InitializeMemory<T>(std::forward<TArgs>(args)...);

			return Allocation;
		}

		template<typename T>
		void Free(MemoryAllocation& allocation) {
			allocation.As<T>()->~T();
			FreeBase(allocation);
		}

		template<typename T>
		void Free(MemoryAllocation allocation, TSize num_elements) {
			T* typed_array = allocation.As<T>();

			for (TSize i = 0; i < num_elements; i++) {
				typed_array[i].~T();
			}

			FreeBase(allocation);
		}

		virtual MemoryAllocation AllocateBase(TSize size) = 0;

		virtual void FreeBase(MemoryAllocation& allocation) = 0;

		virtual void Clear() = 0;

		virtual TSize ComputeAlignedSize(TSize size) { 
			return size; 
		};

		void RebaseAllocation(MemoryAllocation& allocation, IAllocator* allocator) {
			MemoryAllocation new_allocation = allocator->AllocateBase(allocation.Size);
			memcpy(new_allocation.Memory, allocation.Memory, allocation.Size);

			FreeBase(allocation);
		}

		template<typename TAlloc, typename... TArgs>
		static TAlloc Setup(TArgs&&... args) {
			return TAlloc(std::forward<TArgs>(args)...);
		}

	protected:
		IAllocator() {};
	};

}