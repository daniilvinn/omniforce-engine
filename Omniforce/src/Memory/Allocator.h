#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Threading/ConditionalLock.h>

namespace Omni {

	/*
	*	@brief Allocates short-living data, which will be freed in next frame, e.g. events
	*/

	template<bool ThreadSafe = false>
	class OMNIFORCE_API TransientAllocator {
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

		template<typename T, typename... Args>
		[[nodiscard]] T* Allocate(Args&&... args)
		{
			ConditionalLock<ThreadSafe> lock;
			T* ptr = new (m_StackPointer) T(std::forward<Args>(args)...);
			m_StackPointer += sizeof(T);

			return ptr;
		};

		// Since it is stack-based allocator for transient data (which will be freed in next frame), we simply set back 
		void Free() 
		{
			ConditionalLock<ThreadSafe> lock;
			m_StackPointer = m_Stack;
			memset(m_Stack, 0, m_Size);
		}

	private:
		byte* m_Stack;
		byte* m_StackPointer;
		uint64 m_Size;
	};
}