#pragma once

#include <cstdint>

namespace Omni {

	typedef uint32_t uint32;

	struct MemoryAllocation {
		void* Memory;
		uint32 Size;

		const uint32 INVALID_ALLOCATION_SIZE = UINT32_MAX;

		MemoryAllocation()
			: Memory(nullptr), Size(INVALID_ALLOCATION_SIZE) {}

		MemoryAllocation& operator=(const MemoryAllocation& other) {
			Memory = other.Memory;
			Size   = other.Size;

			return *this;
		}

		template<typename T>
		inline T* As() const {
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
	};

}