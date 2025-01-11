#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Memory/MemoryAllocation.h>
#include <Foundation/Assert.h>

namespace Omni {
	/*
	*  @brief A pointer that only holds a weak reference to an object, e.g. it doesn't
	*  own the object so it cannot perform object deallocation
	*/
	template<typename T>
	class OMNIFORCE_API WeakPtr {
	public:
		WeakPtr(MemoryAllocation InAllocation)
			: m_Allocation(InAllocation) {
		}

		inline T* operator->() const {
			return m_Allocation.As<T>();
		}

		inline T* Raw() const {
			return m_Allocation.As<T>();
		}

		template<typename U>
		inline WeakPtr<U> As() const {
			return WeakPtr<U>(m_Allocation);
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

	private:
		MemoryAllocation m_Allocation;
	};

}