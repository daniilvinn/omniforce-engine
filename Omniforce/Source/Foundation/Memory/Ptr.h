#pragma once

#include "../Platform.h"
#include "../BasicTypes.h"

#include "MemoryAllocation.h"
#include "Allocator.h"

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
		: m_Allocation(InAllocation) {}

		inline T* operator->() const {
			return m_Allocation.As<T*>();
		}

		inline const T* Raw() const {
			return (const T*)m_Allocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(m_Allocation);
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

	private:
		MemoryAllocation m_Allocation;
	};

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
		{}

		~Ptr() {
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation);
			}
		}

		// Forbid copying
		Ptr(const Ptr& other) = delete;

		template<typename U>
		Ptr(Ptr<U>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			other.ReleaseNoFree();
		}

		inline void ForceRelease() {
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation);
			}

			m_Allocator = nullptr;
		}

		inline void ReleaseNoFree() {
			m_Allocation.Invalidate();
			m_Allocator = nullptr;
		}

		inline const T* Raw() const {
			return (const T*)m_Allocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(m_Allocation);
		}

		Ptr<T>& operator=(const Ptr<T>& other) = delete;

		template<typename U>
		Ptr<T>& operator=(Ptr<U>&& other) noexcept {
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			if (m_Allocation.IsValid())
				m_Allocator->Free<T>(m_Allocation);

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;

			other.ReleaseNoFree();

			return *this;
		};

		inline T* operator->() {
			return m_Allocation.As<T>();
		}

		inline const T* operator->() const {
			return m_Allocation.As<T>();
		}

		inline T& operator*() {
			return *m_Allocation.As<T>();
		}

		inline operator bool() const {
			return m_Allocation.IsValid();
		}

		template<typename T, typename... TArgs>
		friend Ptr<T> CreatePtr(IAllocator* allocator, TArgs&&... args);

		template<typename U>
		friend class Ptr;

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;

	};

	template<typename T, typename... TArgs>
	Ptr<T> CreatePtr(IAllocator* allocator, TArgs&&... args) {
		return Ptr<T>(allocator, std::forward<TArgs>(args)...);
	};

	template<typename T>
	Ptr<T> CreateInvalidPtr() {
		return Ptr<T>();
	};

	/*
	*  @brief Ref-counted pointer
	*/
	template<typename T>
	class OMNIFORCE_API Ref {
	public:
		using CounterType = uint64;

		struct StorageType {
			template<typename... TArgs>
			StorageType(TArgs&&... args)
				: object(std::forward<TArgs>(args)...), ref_counter(1) {
			}

			Atomic<CounterType> ref_counter;
			T object;
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
				m_CachedObject = (m_Allocation.Memory + sizeof(CounterType));
				m_CachedCounter = m_CounterAllocation.As<Atomic<CounterType>>();

			}
			// Otherwise, make one allocation for both counter and object if size will be the same 
			// (it is likely that transient or dedicated allocators are used)
			else {
				m_Allocation = m_Allocator->Allocate<StorageType>(std::forward<TArgs>(args)...);
				m_CounterAllocation = m_Allocation;

				// Cache object and counter pointers so we don't need to add `sizeof(CounterType)` each time we access an object
				m_CachedObject = (m_Allocation.Memory + sizeof(CounterType));
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

		~Ref() {
			// Decrement ref counter
			DecrementRefCounter();

			// Check if allocation is valid (e.g. if this pointer references some object)
			if (CanReleaseAllocation()) {
				ReleaseAllocation();
			}
		}

		template<typename U>
		Ref(const Ref& other) 
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CounterAllocation(other.m_CounterAllocation)
			, m_CachedObject(other.m_CachedObject)
			, m_CachedCounter(other.m_CachedCounter)
		{
			static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "Types are not related to each other");

			IncrementRefCounter();
		};

		template<typename U>
		Ref(Ptr<U>&& other) noexcept
			: m_Allocator(other.m_Allocator)
			, m_Allocation(other.m_Allocation)
			, m_CounterAllocation(other.m_CounterAllocation)
			, m_CachedObject(other.m_CachedObject)
			, m_CachedCounter(other.m_CachedCounter)
		{
			static_assert(std::is_base_of_v<T, U>, "Types are not related to each other");
			IncrementRefCounter();

			other.Reset();
		}

		inline const T* Raw() const {
			return (const T*)m_Allocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(m_Allocation);
		}

		void Reset() {
			if (m_Allocation.IsValid()) [[likely]] {
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
			m_CachedObject = other.m_CachedObject;
			m_CachedCounter = other.m_CachedCounter;

			IncrementRefCounter();
		}

		template<typename U>
		Ref<T>& operator=(Ref<U>&& other) noexcept {
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

		inline T* operator->() {
			return m_Allocation.As<T>();
		}

		inline const T* operator->() const {
			return m_Allocation.As<T>();
		}

		inline T& operator*() {
			return *m_Allocation.As<T>();
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

			if (*(m_Allocation.As<Atomic<CounterType>>()) != 0) {
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
			(*m_CachedCounter)++;
		}

		inline void DecrementRefCounter() {
			(*m_CachedCounter)--;
		}

	private:
		IAllocator* m_Allocator;

		MemoryAllocation m_Allocation;
		MemoryAllocation m_CounterAllocation;

		T* m_CachedObject;
		Atomic<CounterType>* m_CachedCounter;

	};

	template<typename T, typename... TArgs>
	Ref<T> CreateRef(IAllocator* allocator, TArgs&&... args) {
		return Ref<T>(allocator, std::forward<TArgs>(args)...);
	}

}