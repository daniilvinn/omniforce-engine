#pragma once

#include <cstdint>

namespace Omni {

	typedef uint64_t uint64;
	typedef uint8_t  byte;

	struct MemoryAllocation {
		byte* Memory;
		uint64 Size;

		const uint64 INVALID_ALLOCATION_SIZE = 0;

		MemoryAllocation()
			: Memory(nullptr), Size(INVALID_ALLOCATION_SIZE) {}

		MemoryAllocation(byte* memory, uint64 size)
			: Memory(memory), Size(size) {}

		MemoryAllocation& operator=(const MemoryAllocation& other) {
			Memory = other.Memory;
			Size   = other.Size;

			return *this;
		}

		template<typename T>
		inline constexpr T* As() const {
			return (T*)Memory;
		}

		template<typename T, typename... Args>
		inline void InitializeMemory(Args&&... args) {
			new (Memory) T(std::forward<Args>(args)...);
		}

		inline void Invalidate() {
			Memory = nullptr;
			Size = INVALID_ALLOCATION_SIZE;
		}

		inline bool IsValid() const {
			return Memory != nullptr;
		}
		
		inline static MemoryAllocation InvalidAllocation() { return MemoryAllocation(); }

	};

}