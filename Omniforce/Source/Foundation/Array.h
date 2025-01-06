#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>
#include <Foundation/Log/Logger.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryAllocation.h>

namespace Omni {

	// Resizable array
	template<typename T>
	class OMNIFORCE_API Array {
	public:
		using SizeType = uint64;

		static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,
			"T must be either copy-constructible or move-constructible");

		Array(IAllocator* allocator) 
			: m_Allocator(allocator)
			, m_Size(0)
			, m_GrowthFactor(2.0f) 
		{

		}

		// Num of objects, not size in bytes
		Array(IAllocator* allocator, SizeType size) 
			: m_Allocator(allocator)
			, m_Size(0)
			, m_GrowthFactor(2.0f)
		{
			Resize(size);
		}

		Array(IAllocator* allocator, std::initializer_list<T> init_list)
			: m_Allocator(allocator), m_Size(0), m_GrowthFactor(2.0f) {
			Reallocate(init_list.size());

			T* typed_array = m_Allocation.As<T>();
			for (const auto& value : init_list) {
				new (&typed_array[m_Size++]) T(value);
			}
		}

		~Array() {
			T* typed_array = m_Allocation.As<T>();
			for (uint32 i = 0; i < m_Size; i++) {
				typed_array[i].~T();
			}
			
			if (m_Allocation.IsValid()) {
				m_Allocator->Free<T>(m_Allocation, m_Size);
			}
		}

		void Add(const T& value) {
			if (m_Size == m_MaxSize) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();
			typed_array[m_Size] = value;

			m_Size++;
		}

		void Add(T&& value) {
			if (m_Size == m_MaxSize) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();
			typed_array[m_Size] = value;

			m_Size++;
		}

		void Remove(SizeType index) {
			OMNIFORCE_ASSERT_TAGGED(index < m_Size, "Index out of bounds!");

			T* typed_array = m_Allocation.As<T>();

			typed_array[index].~T();

			for (SizeType i = index; i < m_Size - 1; ++i) {
				typed_array[i] = std::move(typed_array[i + 1]);
			}

			m_Size--;
		}

		void Remove(SizeType from, SizeType to) {
			OMNIFORCE_ASSERT_TAGGED(from < m_Size && to <= m_Size && from <= to, "Invalid range!");

			T* typed_array = m_Allocation.As<T>();

			// Destroy elements in the specified range
			for (SizeType i = from; i < to; ++i) {
				typed_array[i].~T();
			}

			// Shift elements after the range to the left
			SizeType count_to_move = m_Size - to;
			for (SizeType i = 0; i < count_to_move; ++i) {
				typed_array[from + i] = std::move(typed_array[to + i]);
			}

			// Decrement size
			m_Size -= (to - from);
		}

		void Insert(const T& value, SizeType index) {
			OMNIFORCE_ASSERT_TAGGED(index <= m_Size, "Index out of bounds");

			// Check if we are out of bounds
			if (m_Size == m_MaxSize) {
				Reallocate(CalculateNewSize());
			}

			T* typed_array = m_Allocation.As<T>();

			// Shift elements to the right
			for (SizeType i = m_Size; i > index; --i) {
				typed_array[i] = std::move(typed_array[i - 1]);
			}

			// Use in-place constructor
			new (&typed_array[index]) T(value);

			m_Size++;
		}

		void Insert(const Array<T>& range, SizeType index) {
			OMNIFORCE_ASSERT_TAGGED(index <= m_Size, "Index out of bounds!");

			SizeType range_size = range.Size();
			if (range_size == 0) {
				return; // Early out in case if input range is empty
			}

			// Check if we have enough memory
			SizeType new_size = m_Size + range_size;
			if (!HasEnoughMemory(new_size)) {
				Reallocate(new_size);
			}

			T* typed_array = m_Allocation.As<T>();
			T* range_array = range.Raw();

			// Shift elements to the right
			for (SizeType i = m_Size; i > index; --i) {
				typed_array[i + range_size - 1] = std::move(typed_array[i - 1]);
			}

			// Copy elements
			for (SizeType i = 0; i < range_size; ++i) {
				new (&typed_array[index + i]) T(range_array[i]);
			}

			m_Size = new_size;
		}

		void Clear() {
			T* typed_array = m_Allocation.As<T>();

			for (SizeType i = 0; i < m_Size; ++i) {
				typed_array[i].~T();
			}

			m_Size = 0;
		}

		void Resize(SizeType new_size) {
			if (new_size == m_Size) [[unlikely]] {
				// No change in size
				return;
			}

			T* typed_array = m_Allocation.As<T>();

			if (new_size > m_Size) {
				// Growing the array
				if (!HasEnoughMemory(new_size)) {
					// Reallocate memory if needed
					Reallocate(new_size);
					typed_array = m_Allocation.As<T>();
				}

				// Initialize new elements
				for (SizeType i = m_Size; i < new_size; ++i) {
					new (&typed_array[i]) T(); // Placement new to construct default objects
				}
			}
			else {
				// Shrinking the array
				for (SizeType i = new_size; i < m_Size; ++i) {
					typed_array[i].~T(); // Call destructor on removed elements
				}
			}

			// Update the size
			m_Size = new_size;
		}

		inline void	Preallocate(SizeType new_capacity) { 
			Reallocate(new_capacity); 
		}

		inline uint32 Size() const { 
			return m_Size; 
		}

		inline constexpr bool IsEmpty() const { 
			return (bool)m_Size; 
		}

		inline uint32 Capacity() const { 
			return m_Allocation.Size / sizeof(T); 
		}

		inline T* Raw()	const { 
			return m_Allocation.As<T>(); 
		}

		inline void	SetGrowthFactor(float32 factor) { 
			OMNIFORCE_ASSERT_TAGGED(factor > 1.0f, "Growth factor must be greater than 1.0");
			m_GrowthFactor = factor; 
		}

		Array<T>& operator=(const Array<T>& other) {
			if (this == &other) {
				OMNIFORCE_CORE_WARNING("Attempted to assign an array to itself");
				return *this;
			}

			Clear();

			if (HasEnoughMemory(other.Size())) {
				Reallocate(other.Size());
			}

			T* typed_array = m_Allocation.As<T>();
			const T* other_array = other.m_Allocation.As<T>();

			for (SizeType i = 0; i < other.m_Size; ++i) {
				new (&typed_array[i]) T(other_array[i]);
			}

			m_Size = other.m_Size;

			return *this;
		}
		

		Array<T>& operator=(Array<T>&& other) noexcept {
			if (this == &other) {
				return *this;
			}

			Clear();

			//m_Allocator = other.m_Allocator;
			m_Allocation = other.m_Allocation;
			m_Size = other.m_Size;
			m_MaxSize = other.m_MaxSize;
			m_GrowthFactor = other.m_GrowthFactor;

			other.m_Allocation.Invalidate();
			other.m_Size = 0;
			other.m_MaxSize = 0;

			return *this;
		}

		Array<T>& operator=(std::initializer_list<T> init_list) {
			// Reallocate if not enough memory
			if (!HasEnoughMemory(init_list.size())) {
				Reallocate(init_list.size());
			}

			Clear();

			T* typed_array = m_Allocation.As<T>();

			for (const auto& value : init_list) {
				new (&typed_array[m_Size++]) T(value);
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
			return m_Allocation.As<T>() + m_Size; 
		}

		inline const T* begin() const {
			return m_Allocation.As<T>(); 
		}

		inline const T* end() const {
			return m_Allocation.As<T>() + m_Size; 
		}

		inline const T* rbegin() const {
			return m_Allocation.As<T>() + m_Size - 1;
		}

		inline const T* rend() const {
			return m_Allocation.As<T>() - 1;
		}

	private:
		inline SizeType CalculateNewSize() const { 
			return m_Allocation.Size * m_GrowthFactor; 
		}

		inline bool	HasEnoughMemory(SizeType new_size) const { 
			return m_MaxSize >= new_size; 
		}

		void Reallocate(SizeType new_size = 256 / sizeof(T)) {

			MemoryAllocation new_allocation = m_Allocator->AllocateBase(new_size * sizeof(T));
			if (m_Allocation.IsValid()) [[likely]] 
			{
				memcpy(new_allocation.Memory, m_Allocation.Memory, std::min(new_size, m_Size) * sizeof(T));

				m_Allocator->FreeBase(m_Allocation);
				m_MaxSize = new_allocation.Size / sizeof(T);
			}
			m_Allocation = new_allocation;
		}

	private:
		IAllocator* m_Allocator;
		MemoryAllocation m_Allocation;
		SizeType m_Size;
		SizeType m_MaxSize; // Cached value for max size
		float32 m_GrowthFactor;

	};

	using ByteArray = Array<byte>;

}