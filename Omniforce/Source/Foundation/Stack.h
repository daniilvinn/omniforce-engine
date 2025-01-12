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

		Stack(IAllocator* allocator)
			: m_Allocator(allocator)
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{
			Reallocate(32);
		}

		Stack(IAllocator* allocator, TSize size)
			: m_Allocator(allocator)
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{
			Reallocate(size);
		}

		Stack(IAllocator* allocator, std::initializer_list<T> init_list)
			: m_Allocator(allocator)
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{
			Reallocate(init_list.size());

			T* typed_array = m_Allocation.As<T>();
			for (const auto& value : init_list) {
				new (&typed_array[m_Size++]) T(value);
			}
		}

		~Stack() {
			Clear();
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}
		}

		void Push(const T& value) {
			if (m_Size == m_MaxSize) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();
			new (&typed_array[m_Size]) T(value);
			m_Size++;
		}

		void Push(T&& value) {
			if (m_Size == m_MaxSize) {
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

		inline void Preallocate(TSize new_capacity) {
			Reallocate(new_capacity);
		}

		inline TSize Size() const {
			return m_Size;
		}

		inline constexpr bool IsEmpty() const {
			return m_Size == 0;
		}

		inline TSize Capacity() const {
			return m_MaxSize;
		}

		inline void SetGrowthFactor(float32 factor) {
			OMNIFORCE_ASSERT_TAGGED(factor > 1.0f, "Growth factor must be greater than 1.0");
			m_GrowthFactor = factor;
		}

	private:
		inline TSize CalculateNewSize() const {
			return static_cast<TSize>(m_MaxSize * m_GrowthFactor);
		}

		inline bool HasEnoughMemory(TSize new_size) const {
			return m_MaxSize >= new_size;
		}

		void Reallocate(TSize new_size) {
			MemoryAllocation new_allocation = m_Allocator->AllocateBase(new_size * sizeof(T));
			if (m_Allocation.IsValid()) {
				memcpy(new_allocation.Memory, m_Allocation.Memory, std::min(new_size, m_Size) * sizeof(T));
				m_Allocator->FreeBase(m_Allocation);
			}
			m_MaxSize = new_size;
			m_Allocation = new_allocation;
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;
		TSize m_Size;
		TSize m_MaxSize;
		float32 m_GrowthFactor;
	};

}