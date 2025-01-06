#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Log/Logger.h>

namespace Omni {

	/*
	*	@brief Allocates short-living data, which will be freed in next frame, e.g. events.
	*   Has two variants - with- and without thread safety.
	*   The variant with thread safety has nearly free allocation cost
	*/
	template<bool ThreadSafe = false>
	class OMNIFORCE_API TransientAllocator;

	template<>
	class OMNIFORCE_API TransientAllocator<false> : public IAllocator {
	public:
		TransientAllocator(SizeType pool_size = 1024 * 1024)
			: m_Size(0)
			, m_MaxSize(pool_size)
		{
			m_MemoryPool = new byte[pool_size];
		};

		~TransientAllocator()
		{
			delete[] m_MemoryPool;
		};

		MemoryAllocation AllocateBase(SizeType InAllocationSize) override {
			OMNIFORCE_ASSERT_TAGGED(m_MaxSize >= m_Size + InAllocationSize, "Transient allocator: out of memory");

			MemoryAllocation Alloc;
			Alloc.Size = InAllocationSize;
			Alloc.Memory = AllocateMemory(InAllocationSize);

			return Alloc;
		}

		template<typename T, typename... Args>
		[[nodiscard]] T* AllocateObject(Args&&... args)
		{
			OMNIFORCE_ASSERT_TAGGED(m_MaxSize >= m_Size + sizeof(T), "Transient allocator: out of memory");

			T* ptr = new (m_MemoryPool + m_Size) T(std::forward<Args>(args)...);
			m_Size += sizeof(T);

			return ptr;
		};

		[[nodiscard]] byte* AllocateMemory(SizeType size)
		{
			OMNIFORCE_ASSERT_TAGGED(m_MaxSize >= m_Size + size, "Transient allocator: out of memory");

			byte* ptr = m_MemoryPool + m_Size;
			m_Size += sizeof(size);

			return ptr;
		};

		void FreeBase(MemoryAllocation& InAllocation) override {
			// Do nothing except allocation invalidation;
			// All data will be freed once `Clear()` is called
			InAllocation.Invalidate();
		}

		// Since it is stack-based allocator for transient data (which will be freed in next frame), we simply set back 
		void Clear() override
		{
			m_Size = 0;
		}

	private:
		byte* m_MemoryPool = nullptr;
		SizeType m_Size = 0;
		SizeType m_MaxSize = 0;
	};

	template<>
	class OMNIFORCE_API TransientAllocator<true> : public IAllocator {
	public:

		TransientAllocator(SizeType pool_size = 1024 * 1024)
			: m_Size(0)
			, m_MaxSize(pool_size)
		{
			m_MemoryPool = new byte[pool_size];
		};

		~TransientAllocator()
		{
			delete[] m_MemoryPool;
		};

		MemoryAllocation AllocateBase(SizeType InAllocationSize) override {
			MemoryAllocation Alloc;

			Alloc.Size = InAllocationSize;
			Alloc.Memory = AllocateMemory(InAllocationSize);

			return Alloc;
		}

		template<typename T, typename... Args>
		[[nodiscard]] T* AllocateObject(Args&&... args)
		{
			OMNIFORCE_ASSERT_TAGGED(m_MaxSize >= m_Size + sizeof(T), "Transient allocator: out of memory");

			SizeType ptr_offset = m_Size.fetch_add(sizeof(T));
			T* ptr = new (m_MemoryPool + ptr_offset) T(std::forward<Args>(args)...);

			return ptr;
		};

		[[nodiscard]] byte* AllocateMemory(SizeType size)
		{
			OMNIFORCE_ASSERT_TAGGED(m_MaxSize >= m_Size + size, "Transient allocator: out of memory");

			SizeType ptr_offset = m_Size.fetch_add(size);
			byte* ptr = m_MemoryPool + ptr_offset;

			return ptr;
		};

		void FreeBase(MemoryAllocation& InAllocation) override {
			// Do nothing except allocation invalidation;
			// All data will be freed once `Clear()` is called
			InAllocation.Invalidate();
		}

		// Since it is stack-based allocator for transient data (which will be freed in next frame), 
		// we simply set back pointer offset
		void Clear() override {
			m_Size = 0;
		}

		SizeType ComputeAlignedSize(SizeType size) override { 
			return size; 
		};

	private:
		byte* m_MemoryPool = nullptr;
		Atomic<SizeType> m_Size = 0;
		SizeType m_MaxSize = 0;
	};

	/*
	*  @brief Global allocator for data that lives one frame. Managed by the engine. Do not call `Clear()`
	*/
	inline TransientAllocator<true> g_TransientAllocator = IAllocator::Setup<TransientAllocator<true>>();

}