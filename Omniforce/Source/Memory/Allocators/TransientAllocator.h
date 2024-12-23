#pragma once

#include "../Allocator.h"

#include <Foundation/Types.h>

namespace Omni {

	/*
	*	@brief Allocates short-living data, which will be freed in next frame, e.g. events
	*/
	template<bool ThreadSafe = false>
	class OMNIFORCE_API TransientAllocator : public IAllocator {
	public:

		TransientAllocator(uint64 pool_size = 1024 * 1024)
			: m_Size(pool_size)
		{
			m_Stack = new byte[pool_size]; // Allocating 1MB
			m_StackPointer = m_Stack; // bottom of the stack
		};

		~TransientAllocator()
		{
			delete[] m_Stack;
		};

		MemoryAllocation AllocateBase(uint32 InAllocationSize) override {
			MemoryAllocation Alloc;
			Alloc.Size = InAllocationSize;
			Alloc.Memory = AllocateMemory(InAllocationSize);

			return Alloc;
		}

		template<typename T, typename... Args>
		[[nodiscard]] T* AllocateObject(Args&&... args)
		{
			m_Mutex.Lock();
			T* ptr = new (m_StackPointer) T(std::forward<Args>(args)...);
			m_StackPointer += sizeof(T);
			m_Mutex.Unlock();

			return ptr;
		};

		[[nodiscard]] byte* AllocateMemory(uint32 InSize)
		{
			m_Mutex.Lock();
			byte* ptr = m_StackPointer;
			m_StackPointer += sizeof(InSize);
			m_Mutex.Unlock();

			return ptr;
		};

		void Free(MemoryAllocation& InAllocation) override {

		}

		// Since it is stack-based allocator for transient data (which will be freed in next frame), we simply set back 
		void Clear() override
		{
			m_Mutex.Lock();
			m_StackPointer = m_Stack;
			memset(m_Stack, 0, m_Size);
			m_Mutex.Unlock();
		}

	private:
		byte* m_Stack;
		byte* m_StackPointer;
		uint64 m_Size;
		ConditionalLock<ThreadSafe> m_Mutex;
	};

}