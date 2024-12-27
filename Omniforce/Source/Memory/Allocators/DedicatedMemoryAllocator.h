#pragma once

#include "../Allocator.h"
#include "../InternalSetup.h"
#include "../Pointers.h"

namespace Omni {

	/*
	*	@brief Allocated a dedicated memory block for a resource. 
	*	Useful for large, persistent resources, such as images
	*/
	class OMNIFORCE_API DedicatedMemoryAllocator : public IAllocator {
	public:
		DedicatedMemoryAllocator() {};

		MemoryAllocation AllocateBase(uint32 InAllocationSize) override {
			MemoryAllocation Allocation;
			Allocation.Memory = new byte[InAllocationSize];
			Allocation.Size = InAllocationSize;

			return Allocation;
		}

		void Free(MemoryAllocation& InAllocation) override {
			delete[] InAllocation.Memory;
			InAllocation.Invalidate();
		}

		void Clear() override {};

	};

	inline DedicatedMemoryAllocator GDedicatedMemoryAllocator = IAllocator::Setup<DedicatedMemoryAllocator>();

}