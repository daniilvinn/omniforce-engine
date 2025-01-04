#pragma once

#include "../Allocator.h"
#include "../Ptr.h"

namespace Omni {

	/*
	*	@brief Allocated a dedicated memory block for a resource. 
	*	Useful for large, persistent resources, such as images
	*/
	class OMNIFORCE_API DedicatedMemoryAllocator : public IAllocator {
	public:
		DedicatedMemoryAllocator() {};

		MemoryAllocation AllocateBase(SizeType InAllocationSize) override {
			MemoryAllocation Allocation;
			Allocation.Memory = new byte[InAllocationSize];
			Allocation.Size = InAllocationSize;

			return Allocation;
		}

		void FreeBase(MemoryAllocation& InAllocation) override {
			delete[] InAllocation.Memory;
			InAllocation.Invalidate();
		}

		void Clear() override {};

		SizeType ComputeAlignedSize(SizeType size) override { 
			return size; 
		};

	};

	inline DedicatedMemoryAllocator g_DedicatedMemoryAllocator = IAllocator::Setup<DedicatedMemoryAllocator>();

}