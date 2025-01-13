#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Log/Logger.h>
#include <Foundation/Assert.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryAllocation.h>

namespace Omni {

	// Resizable array
	template<typename T, uint64 TSize>
	requires ((std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>) && !std::is_abstract_v<T> && std::is_default_constructible_v<T>)
	class OMNIFORCE_API StaticArray {
	public:
		using SizeType = uint64;

		StaticArray(IAllocator* allocator)
			: m_Allocator(allocator)
		{
			m_Allocation = allocator->AllocateBase(TSize * sizeof(T));

			T* typed_array = m_Allocation.As<T>();

			for (SizeType i = 0; i < TSize; ++i) {
				new (&typed_array[i]) T();
			}
		}

		StaticArray(IAllocator* allocator, std::initializer_list<T> init_list)
			: m_Allocator(allocator)
		{
			OMNIFORCE_ASSERT_TAGGED(TSize >= init_list.size(), "Attempted to assign initializer list that is bigger than array size");

			m_Allocation = allocator->AllocateBase(TSize * sizeof(T));

			T* typed_array = m_Allocation.As<T>();

			for (SizeType i = 0; i < TSize; i++) {
				if (i < init_list.size()) {
					new (&typed_array[i]) T(init_list[i]);
				}
				else {
					new (&typed_array[i]) T();
				}
			}
		}

		~StaticArray() {
			if (m_Allocation.IsValid()) {
				T* typed_array = m_Allocation.As<T>();
				for (uint32 i = 0; i < TSize; i++) {
					typed_array[i].~T();
				}
				m_Allocator->Free<T>(m_Allocation, TSize);
			}
		}

		void Clear() {
			T* typed_array = m_Allocation.As<T>();

			for (SizeType i = 0; i < TSize; i++) {
				typed_array[i].~T();
			}
		}

		inline constexpr uint32 Size() const {
			return TSize;
		}

		inline T* Raw()	const {
			return m_Allocation.As<T>();
		}

		StaticArray& operator=(const StaticArray<T, TSize>& other) {
			if (this == &other) {
				OMNIFORCE_CORE_WARNING("Attempted to assign an array to itself");
				return *this;
			}

			Clear();

			T* typed_array = m_Allocation.As<T>();
			const T* other_array = other.m_Allocation.As<T>();

			for (SizeType i = 0; i < TSize; i++) {
				new (&typed_array[i]) T(other_array[i]);
			}

			return *this;
		}


		Array<T>& operator=(Array<T>&& other) noexcept {
			if (this == &other) {
				return *this;
			}

			Clear();

			//m_Allocator = other.m_Allocator;
			m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;

			other.m_Allocation.Invalidate();
			other.m_Size = 0;
			other.m_MaxSize = 0;

			return *this;
		}

		Array<T>& operator=(std::initializer_list<T> init_list) {
			OMNIFORCE_ASSERT_TAGGED(TSize >= init_list.size(), "Attempted to assign initializer list that is bigger than array size");

			T* typed_array = m_Allocation.As<T>();

			for (SizeType i = 0; i < TSize; i++) {
				if (i < init_list.size()) {
					new (&typed_array[i]) T(init_list[i]);
				}
				else {
					new (&typed_array[i]) T();
				}
			}

			return *this;
		}

		inline T& operator[](uint32 index) {
			return m_Allocation.As<T>()[index];
		}

		inline const T& operator[](uint32 index) const {
			return m_Allocation.As<T>()[index];
		}

		inline T* begin() {
			return m_Allocation.As<T>();
		}

		inline T* end() {
			return m_Allocation.As<T>() + TSize;
		}

		inline const T* begin() const {
			return m_Allocation.As<T>();
		}

		inline const T* end() const {
			return m_Allocation.As<T>() + TSize;
		}

		inline const T* rbegin() const {
			return m_Allocation.As<T>() + TSize - 1;
		}

		inline const T* rend() const {
			return m_Allocation.As<T>() - 1;
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;
	};

}