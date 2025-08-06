#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Memory/MemoryAllocation.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Assert.h>

namespace Omni {
	/*
	*  @brief Smart pointer that performs auto-deletion of an object
	*  once pointer is out of the scope the destructor is invoked
	*/
	template<typename T>
	class OMNIFORCE_API Ptr {

		template<typename... TArgs>
		Ptr(IAllocator* allocator, TArgs&&... args)
			: m_Allocator(allocator) {
			m_Allocation = m_Allocator->Allocate<T>(std::forward<TArgs>(args)...);
		};

	public:
		Ptr()
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
		{
		}

		~Ptr() {
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation);
			}
		}

		// Forbid copying
		Ptr(const Ptr& other) = delete;

		// Allow only nullptr to be assigned when it comes to assigning a raw pointer
		Ptr(std::nullptr_t) {
			m_Allocation = MemoryAllocation::InvalidAllocation();
			m_Allocator = nullptr;
		}

		template<typename T>
		Ptr(T*) = delete;

		Ptr(Ptr<T>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
		{
			other.ReleaseNoFree();
		}

		template<typename U>
		Ptr(Ptr<U>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || !std::is_abstract_v<U>, "Types are not related to each other");

			other.ReleaseNoFree();
		}

		inline void ForceRelease() {
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation);
			}

			m_Allocator = nullptr;
		}

		inline constexpr T* Raw() const {
			return m_Allocation.As<T>();
		}

		Ptr<T>& operator=(const Ptr<T>& other) = delete;

		Ptr<T>& operator=(Ptr<T>&& other) noexcept {
			if (m_Allocation.IsValid())
				m_Allocator->Free<T>(m_Allocation);

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;

			other.ReleaseNoFree();

			return *this;
		};

		template<typename U>
		Ptr<T>& operator=(Ptr<U>&& other) noexcept {
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || !std::is_abstract_v<U>, "Types are not related to each other");

			if (m_Allocation.IsValid())
				m_Allocator->Free<T>(m_Allocation);

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;

			other.ReleaseNoFree();

			return *this;
		};

		inline constexpr T* operator->() {
			return m_Allocation.As<T>();
		}

		inline constexpr const T* operator->() const {
			return m_Allocation.As<T>();
		}

		inline T& operator*() {
			return *m_Allocation.As<T>();
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

		template<typename U, typename... TArgs>
		friend Ptr<U> CreatePtr(IAllocator* allocator, TArgs&&... args);

		template<typename U>
		friend class Ptr;

	private:
		inline void ReleaseNoFree() {
			m_Allocation.Invalidate();
			m_Allocator = nullptr;
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;

	};

	template<typename U, typename... TArgs>
	Ptr<U> CreatePtr(IAllocator* allocator, TArgs&&... args) {
		return Ptr<U>(allocator, std::forward<TArgs>(args)...);
	};

	template<typename T>
	Ptr<T> CreateInvalidPtr() {
		return Ptr<T>();
	};

}