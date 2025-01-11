#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Memory/MemoryAllocation.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/WeakPtr.h>
#include <Foundation/Assert.h>

namespace Omni {
	/*
	*  @brief Ref-counted pointer
	*/
	template<typename T>
	class OMNIFORCE_API Ref {
	public:
		using CounterType = uint64;

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

			Atomic<CounterType> ref_counter;
			byte object[sizeof(T)]; // a hack that forbids allocation of T explicitly
		};

		template<typename... TArgs>
		Ref(IAllocator* allocator, TArgs&&... args)
			: m_Allocator(allocator)
		{
			// Split allocations if size will be different if we don't split them.
			// In this case, it is likely that persistent allocator is used and it will be able to make additional splits
			// that may be used for other counters as well
			if (m_Allocator->ComputeAlignedSize(sizeof(StorageType)) != m_Allocator->ComputeAlignedSize(sizeof(T))
				+ m_Allocator->ComputeAlignedSize(sizeof(CounterType)))
			{
				m_Allocation = m_Allocator->Allocate<T>(std::forward<TArgs>(args)...);
				m_CounterAllocation = m_Allocator->Allocate<CounterType>(1);

				// Cache pointers
				m_CachedObject = m_Allocation.As<T>();
				m_CachedCounter = m_CounterAllocation.As<Atomic<CounterType>>();

			}
			// Otherwise, make one allocation for both counter and object if size will be the same 
			// (it is likely that transient or dedicated allocators are used)
			else {
				m_Allocation = m_Allocator->Allocate<StorageType>(std::forward<TArgs>(args)...);
				m_CounterAllocation = m_Allocation;

				// Cache object and counter pointers so we don't need to add `sizeof(CounterType)` each time we access an object
				m_CachedObject = (T*)(m_Allocation.Memory + sizeof(CounterType));
				m_CachedCounter = m_Allocation.As<Atomic<CounterType>>();
			}
		};

	public:
		Ref()
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_CounterAllocation(MemoryAllocation::InvalidAllocation())
			, m_CachedObject(nullptr)
			, m_CachedCounter(nullptr)
		{
		}

		// Make it only possible to accept nullptr
		Ref(std::nullptr_t)
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_CounterAllocation(MemoryAllocation::InvalidAllocation())
			, m_CachedObject(nullptr)
			, m_CachedCounter(nullptr)
		{
		}

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
			, m_CounterAllocation(other.m_CounterAllocation)
			, m_CachedObject((T*)other.m_CachedObject)
			, m_CachedCounter(other.m_CachedCounter)
		{
			IncrementRefCounter();
		};

		template<typename U>
		Ref(const Ref<U>& other)
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CounterAllocation(other.m_CounterAllocation)
			, m_CachedObject((T*)other.m_CachedObject)
			, m_CachedCounter(other.m_CachedCounter)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			IncrementRefCounter();
		};

		template<typename U>
		Ref(Ref<U>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CounterAllocation(other.m_CounterAllocation)
			, m_CachedObject(other.m_CachedObject)
			, m_CachedCounter(other.m_CachedCounter)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>, "Types are not related to each other");

			IncrementRefCounter();

			other.Reset();
		}

		inline constexpr T* Raw() const {
			return m_CachedObject;
		}

		template<typename U>
		inline constexpr WeakPtr<U> As() const {
			MemoryAllocation allocation = m_Allocation;

			if (m_Allocation == m_CounterAllocation) {
				allocation.Memory += sizeof(uint64);
				allocation.Size -= sizeof(uint64);
			}

			return WeakPtr<U>(allocation);
		}

		void Reset() {
			DecrementRefCounter();

			m_Allocator = nullptr;
			m_Allocation = MemoryAllocation::InvalidAllocation();
			m_CounterAllocation = MemoryAllocation::InvalidAllocation();
			m_CachedObject = nullptr;
			m_CachedCounter = nullptr;

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
			m_CounterAllocation = other.m_CounterAllocation;
			m_CachedObject = (T*)other.m_CachedObject;
			m_CachedCounter = other.m_CachedCounter;

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
			m_CounterAllocation = other.m_CounterAllocation;
			m_CachedObject = (T*)other.m_CachedObject;
			m_CachedCounter = other.m_CachedCounter;

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
			m_CounterAllocation = other.m_CounterAllocation;
			m_CachedObject = other.m_CachedObject;
			m_CachedCounter = other.m_CachedCounter;

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
			m_CounterAllocation = other.m_CounterAllocation;
			m_CachedObject = other.m_CachedObject;
			m_CachedCounter = other.m_CachedCounter;

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

			if (*m_CachedCounter != 0) {
				return false;
			}

			return true;
		}

		inline void ReleaseAllocation() {
			// Free resource
			// Check if counter and object allocation were split
			if (m_CounterAllocation == m_Allocation) {
				m_Allocator->Free<StorageType>(m_Allocation);
			}
			// Otherwise clear both allocations
			else {
				m_Allocator->Free<T>(m_Allocation);
				m_Allocator->Free<CounterType>(m_Allocation);
			}
		}

		inline void IncrementRefCounter() {
			if (m_Allocation.IsValid()) {
				(*m_CachedCounter)++;
			}
		}

		inline void DecrementRefCounter() {
			if (m_Allocation.IsValid()) {
				(*m_CachedCounter)--;
			}
		}

	private:
		IAllocator* m_Allocator;

		MemoryAllocation m_Allocation;
		MemoryAllocation m_CounterAllocation;

		T* m_CachedObject;
		Atomic<CounterType>* m_CachedCounter;

	};

	template<typename T, typename... TArgs>
		requires (!std::is_abstract_v<T>)
	Ref<T> CreateRef(IAllocator* allocator, TArgs&&... args) {
		return Ref<T>(allocator, std::forward<TArgs>(args)...);
	}

}