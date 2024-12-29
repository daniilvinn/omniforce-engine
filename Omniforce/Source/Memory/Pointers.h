#pragma once

#include "MemoryAllocation.h"
#include "Memory/Allocator.h"

#include <memory>
#include <iostream>

namespace Omni {

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T>
	using Shared = std::shared_ptr<T>;

	template <typename T1, typename T2>
	Shared<T1> ShareAs(Shared<T2>& ptr)
	{
		return std::static_pointer_cast<T1>(ptr);
	};

	// A pointer that only holds a reference to an object,
	// but doesn't perform deallocation once it is out of the scope
	template<typename T>
	class OMNIFORCE_API WeakPtr {
	public:
		WeakPtr(MemoryAllocation InAllocation)
		: mAllocation(InAllocation) {}

		inline T* operator->() const {
			return mAllocation.As<T*>();
		}

		inline const T* Raw() const {
			return (const T*)mAllocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(mAllocation);
		}

		inline operator bool() const {
			return mAllocation.IsValid();
		}

	private:
		MemoryAllocation mAllocation;
	};

	// Smart pointer that performs auto-deletion of an object
	// once pointer is out of the scope and destructor is invoked
	template<typename T>
	class OMNIFORCE_API Ptr {

		template<typename... TArgs>
		Ptr(IAllocator* allocator, TArgs&&... args)
			: m_Allocator(allocator) {
			m_Allocation = m_Allocator.Allocate<T>(std::forward<TArgs>(args)...);
		};

	public:
		~Ptr() {
			if (m_Allocation.IsValid()) {
				m_Allocator.Free<T>(m_Allocation);
			}
		}

		template <typename TAlloc, typename... TArgs>
		static auto Create(TAlloc* allocator, TArgs&&... args) {
			return Ptr<T>(allocator, std::forward<TArgs>(args)...);
		}

		Ptr(const Ptr& other) = delete;
		Ptr(Ptr&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
		{
			other.m_Allocation.Invalidate();
		}

		Ptr<T>& operator=(const Ptr<T>& other) = delete;
		Ptr<T>& operator=(Ptr<T>&& other) noexcept {
			if (m_Allocation.IsValid())
				m_Allocator.FreeBase(m_Allocation);

			m_Allocator = other.mAllocator;
			m_Allocation = other.mAllocation;

			other.mAllocation.Invalidate();

			return *this;
		};

		inline T* operator->() {
			return m_Allocation.As<T>();
		}

		inline T& operator*() {
			return *m_Allocation.As<T>();
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

		inline const T* Raw() const {
			return (const T*)m_Allocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(m_Allocation);
		}

	private:
		MemoryAllocation m_Allocation;
		IAllocator* m_Allocator;

	};

}