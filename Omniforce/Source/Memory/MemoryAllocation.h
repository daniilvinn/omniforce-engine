#pragma once

namespace Omni {
	using uint32 = uint32_t;

	struct MemoryAllocation {
		MemoryAllocation()
			: Memory(nullptr), Size(0) {}

		void* Memory;
		uint32 Size;

		template<typename T>
		T* As() const {
			return (T*)Memory;
		}

		template<typename T, typename... Args>
		void InitializeMemory(Args&&... args) {
			new (Memory) T(std::forward<Args>(args)...);
		}

		void Invalidate() {
			Memory = nullptr;
			Size = UINT32_MAX;
		}
	};
}