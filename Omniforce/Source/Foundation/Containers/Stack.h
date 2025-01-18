#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Log/Logger.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryAllocation.h>

namespace Omni {

	template <typename T>
	requires (std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>)
	class OMNIFORCE_API Stack {
	public:
		using TSize = uint64;

		Stack() 
			: m_Allocator(nullptr)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_Size(0)
			, m_GrowthFactor(2.0f)
		{}

		Stack(IAllocator* allocator)
			: m_Allocator(allocator)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{
			Reallocate(32);
		}

		Stack(IAllocator* allocator, std::initializer_list<T> init_list)
			: m_Allocator(allocator)
			, m_Allocation(MemoryAllocation::InvalidAllocation())
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{
			Reallocate(init_list.size());

			T* typed_array = m_Allocation.As<T>();
			for (const auto& value : init_list) {
				new (&typed_array[m_Size++]) T(value);
			}
		}

		Stack(const Stack& other) {
			if (m_Allocation.IsValid()) {
				Clear();
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}

			m_Allocator = other.m_Allocator;
			m_GrowthFactor = other.m_GrowthFactor;
			m_Size = other.m_Size;

			m_Allocation = m_Allocator->AllocateBase(m_Size * sizeof(T));
			
			for (TSize i = 0; i < m_Size; i++) {
				new (&m_Allocation.As<T>()[i]) T(other.m_Allocation.As<T>()[i]);
			}

		}

		Stack(Stack&& other) {
			if (m_Allocation.IsValid()) {
				Clear();
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_GrowthFactor = other.m_GrowthFactor;
			m_Size = other.m_Size;

			other.m_Allocator = nullptr;
			other.m_Allocation.Invalidate();
			other.m_Size = 0;
			other.m_GrowthFactor = 2.0f;

		}

		~Stack() {
			if (m_Allocation.IsValid()) {
				Clear();
				m_Allocator->FreeBase(m_Allocation);
			}
		}

		void Push(const T& value) {
			if (m_Size == Capacity()) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();
			new (&typed_array[m_Size]) T(value);
			m_Size++;
		}

		void Push(T&& value) {
			if (m_Size == Capacity()) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();
			new (&typed_array[m_Size]) T(std::move(value));
			m_Size++;
		}

		void Pop() {
			OMNIFORCE_ASSERT_TAGGED(m_Size > 0, "Cannot pop from an empty stack");

			T* typed_array = m_Allocation.As<T>();
			typed_array[m_Size - 1].~T();
			m_Size--;
		}

		T Top() const {
			OMNIFORCE_ASSERT_TAGGED(m_Size > 0, "Cannot access top element of an empty stack");

			T* typed_array = m_Allocation.As<T>();
			
			return typed_array[m_Size - 1];
		}

		void Clear() {
			T* typed_array = m_Allocation.As<T>();
			for (TSize i = 0; i < m_Size; ++i) {
				typed_array[i].~T();
			}
			m_Size = 0;
		}

		void Reallocate(TSize new_size) {
			MemoryAllocation new_allocation = m_Allocator->AllocateBase(new_size * sizeof(T));

			if (m_Allocation.IsValid()) {
				T* typed_array = (T*)m_Allocation.Memory;
				T* new_typed_array = (T*)new_allocation.Memory;

				for (uint64 i = 0; i < m_Size; i++) {
					// NOTE: May be suboptimal, needs investigation
					memset(new_typed_array + i, 0, sizeof(T));
					new_typed_array[i] = std::move(typed_array[i]);
				}

				m_Allocator->FreeBase(m_Allocation);
			}

			m_Allocation = new_allocation;
		}

		inline TSize Size() const {
			return m_Size;
		}

		Stack& operator=(const Stack& other) {
			if (m_Allocation.IsValid()) {
				Clear();
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}

			m_Allocator = other.m_Allocator;
			m_GrowthFactor = other.m_GrowthFactor;
			m_Size = other.m_Size;

			m_Allocation = m_Allocator->AllocateBase(m_Size * sizeof(T));

			for (TSize i = 0; i < m_Size; i++) {
				new (&m_Allocation.As<T>()[i]) T(other.m_Allocation.As<T>()[i]);
			}

			return *this;
		}

		Stack& operator=(Stack&& other) {
			if (m_Allocation.IsValid()) {
				Clear();
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}

			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_GrowthFactor = other.m_GrowthFactor;
			m_Size = other.m_Size;

			other.m_Allocator = nullptr;
			other.m_Allocation.Invalidate();
			other.m_Size = 0;
			other.m_GrowthFactor = 2.0f;

			return *this;
		}

		inline constexpr bool IsEmpty() const {
			return m_Size == 0;
		}

		inline TSize Capacity() const {
			return m_Allocation.Size / sizeof(T);
		}

		inline void SetGrowthFactor(float32 factor) {
			OMNIFORCE_ASSERT_TAGGED(factor > 1.0f, "Growth factor must be greater than 1.0");
			m_GrowthFactor = factor;
		}

	private:
		inline TSize CalculateNewSize() const {
			return static_cast<TSize>(Capacity() * m_GrowthFactor);
		}

		inline bool HasEnoughMemory(TSize new_size) const {
			return Capacity() >= new_size;
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;
		TSize m_Size;
		float32 m_GrowthFactor;
	};

}