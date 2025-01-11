#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Memory/MemoryAllocation.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Assert.h>

namespace Omni {
	/*
	*  @brief Ref-counted pointer
	*/
	template<typename T>
	class OMNIFORCE_API Ref {
	public:
		using TCounter = uint64;

		struct StorageType {
			template<typename... TArgs>
			requires (!std::is_abstract_v<T>)
			StorageType(TArgs&&... args)
				: object({}), ref_counter(1)
			{
				new (object) T(std::forward<TArgs>(args)...);
			}

			template<typename... TArgs>
			requires std::is_abstract_v<T>
			StorageType(TArgs&&... args)
				: object({}), ref_counter(1)
			{
				OMNIFORCE_ASSERT_TAGGED(false, "Cannot instantiate abstract class");
			}

			~StorageType()
			{
				T* typed_object = (T*)object;
				typed_object->~T();
			}

			Atomic<TCounter> ref_counter;
			byte object[sizeof(T)]; // a hack that forbids allocation of T explicitly
		};

		template<typename... TArgs>
		Ref(IAllocator* allocator, TArgs&&... args)
			: m_Allocator(allocator)
		{
			// Make one allocation for both counter and object so we get better caching
			m_Allocation = m_Allocator->Allocate<StorageType>(std::forward<TArgs>(args)...);

			// Cache object and pointer so we don't need to add `sizeof(CounterType)` each time we access an object
			m_CachedObject = (T*)(m_Allocation.Memory + sizeof(TCounter));
		};

	public:
		Ref()
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_CachedObject(nullptr)
		{
		}

		// Make it only possible to accept nullptr
		Ref(std::nullptr_t)
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_CachedObject(nullptr)
		{}

		// Delete all other variations except nullptr
		template<typename U>
		Ref(U*) = delete;

		~Ref() {
			// Decrement ref counter
			DecrementRefCounter();

			// Check if allocation is valid (e.g. if this pointer references some object)
			if (CanReleaseAllocation()) {
				ReleaseAllocation();
			}
		}

		Ref(const Ref& other)
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CachedObject((T*)other.m_CachedObject)
		{
			IncrementRefCounter();
		};

		template<typename U>
		Ref(const Ref<U>& other)
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CachedObject((T*)other.m_CachedObject)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			IncrementRefCounter();
		};

		template<typename U>
		Ref(Ref<U>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CachedObject(other.m_CachedObject)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>, "Types are not related to each other");

			IncrementRefCounter();

			other.Reset();
		}

		inline constexpr T* Raw() const {
			return m_CachedObject;
		}

		void Reset() {
			DecrementRefCounter();

			m_Allocator = nullptr;
			m_Allocation = MemoryAllocation::InvalidAllocation();
			m_CachedObject = nullptr;

			if (CanReleaseAllocation()) {
				ReleaseAllocation();
			}
		}

		Ref& operator=(const Ref& other) noexcept {
			// Early out in case if two pointers are referencing the same resource
			if (m_Allocation == other.m_Allocation) [[unlikely]] {
				return *this;
			}

			if (m_Allocation.IsValid()) {
				DecrementRefCounter();

				if (CanReleaseAllocation()) {
					ReleaseAllocation();
				}
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_CachedObject = (T*)other.m_CachedObject;

			IncrementRefCounter();

			return *this;
		}

		template<typename U>
		Ref<T>& operator=(const Ref<U>& other) noexcept {
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			// Early out in case if two pointers are referencing the same resource
			if (m_Allocation == other.m_Allocation) [[unlikely]] {
				return *this;
			}

			if (m_Allocation.IsValid()) {
				DecrementRefCounter();

				if (CanReleaseAllocation()) {
					ReleaseAllocation();
				}
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_CachedObject = (T*)other.m_CachedObject;

			IncrementRefCounter();

			return *this;
		}

		Ref& operator=(Ref&& other) noexcept {
			// Early out in case if two pointers are referencing the same resource
			if (m_Allocation == other.m_Allocation) [[unlikely]] {
				return *this;
			}

			if (m_Allocation.IsValid()) {
				DecrementRefCounter();

				if (CanReleaseAllocation()) {
					ReleaseAllocation();
				}
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_CachedObject = other.m_CachedObject;

			IncrementRefCounter();

			other.Reset();

			return *this;
		}

		template<typename U>
		Ref<T>& operator=(Ref<U>&& other) noexcept {
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			// Early out in case if two pointers are referencing the same resource
			if (m_Allocation == other.m_Allocation) [[unlikely]] {
				return *this;
			}

			if (m_Allocation.IsValid()) {
				DecrementRefCounter();

				if (CanReleaseAllocation()) {
					ReleaseAllocation();
				}
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_CachedObject = other.m_CachedObject;

			IncrementRefCounter();

			other.Reset();

			return *this;
		};

		inline T* operator->() const {
			return m_CachedObject;
		}

		inline T& operator*() {
			return *m_CachedObject;
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

		template<typename T, typename... TArgs>
		friend Ref<T> CreateRef(IAllocator* allocator, TArgs&&... args);

		template<typename U>
		friend class Ref;

	private:
		inline bool CanReleaseAllocation() const noexcept {
			// Check if allocation is valid (e.g. if this pointer references some object)
			if (!m_Allocation.IsValid()) {
				return false;
			}

			if (*m_Allocation.As<Atomic<TCounter>>() != 0) { // for some reason fails to compile here
				return false;
			}

			return true;
		}

		inline void ReleaseAllocation() {
			// Free resource
			m_Allocator->Free<StorageType>(m_Allocation);
		}

		inline void IncrementRefCounter() {
			if (m_Allocation.IsValid()) {
				(*m_Allocation.As<Atomic<TCounter>>())++;
			}
		}

		inline void DecrementRefCounter() {
			if (m_Allocation.IsValid()) {
				(*m_Allocation.As<Atomic<TCounter>>())--;
			}
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;
		T* m_CachedObject;
	};

	template<typename T, typename... TArgs>
		requires (!std::is_abstract_v<T>)
	Ref<T> CreateRef(IAllocator* allocator, TArgs&&... args) {
		return Ref<T>(allocator, std::forward<TArgs>(args)...);
	}

}