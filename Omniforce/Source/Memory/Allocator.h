#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Threading/ConditionalLock.h>
#include "MemoryAllocation.h"

namespace Omni {

	class OMNIFORCE_API IAllocator {
	public:
		virtual ~IAllocator() {};

		template<typename T, typename... Args>
		void Allocate(Args&&... args) {
			MemoryAllocation Allocation = AllocateBase(sizeof(T));
			Allocation.InitializeMemory<T>(args);

			return Allocation;
		}

		virtual MemoryAllocation AllocateBase(uint32 InAllocationSize) = 0;

		virtual void Free(MemoryAllocation& InAllocation) = 0;
		virtual void Clear() = 0;

	protected:
		IAllocator() {};
	};

}