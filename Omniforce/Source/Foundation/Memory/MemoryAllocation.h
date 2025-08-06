#pragma once

#include <Foundation/BasicTypes.h>
#include <Foundation/Assert.h>

namespace Omni {

	struct MemoryAllocation {
		using byte = uint8_t;
		using uint64 = uint64_t;

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

		bool operator==(const MemoryAllocation& other) const {
			return (Memory == other.Memory) && (Size == other.Size);
		}

		template<typename T>
		inline constexpr T* As() const {
			return (T*)Memory;
		}

		template<typename T, typename... Args>
		requires (!std::is_abstract_v<T>)
		inline void InitializeMemory(Args&&... args) {
			new (Memory) T(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		requires std::is_abstract_v<T>
		inline void InitializeMemory(Args&&... args) {
			OMNIFORCE_ASSERT_TAGGED(false, "Cannot instantiate abstract class");
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