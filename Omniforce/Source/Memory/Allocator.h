#pragma once

#include <Foundation/Macros.h>
#include "MemoryAllocation.h"

namespace Omni {

	class OMNIFORCE_API IAllocator {
	public:
		virtual ~IAllocator() {};

		template<typename T, typename... TArgs>
		MemoryAllocation Allocate(TArgs&&... args) {
			MemoryAllocation Allocation = AllocateBase(sizeof(T));
			Allocation.InitializeMemory<T>(std::forward<TArgs>(args)...);

			return Allocation;
		}


		virtual void Free(MemoryAllocation& InAllocation) = 0;
		virtual void Clear() = 0;

		template<typename TAlloc, typename... TArgs>
		static TAlloc Setup(TArgs&&... InArgs) {
			return TAlloc(std::forward<TArgs>(InArgs)...);
		}

	protected:
		virtual MemoryAllocation AllocateBase(uint32 InAllocationSize) = 0;

		IAllocator() {};
	};

}