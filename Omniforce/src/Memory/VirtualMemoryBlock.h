#pragma once

#include <Foundation/Types.h>

namespace Omni {

	class VirtualMemoryBlock {
	protected:
		VirtualMemoryBlock() {};

	public:
		static Scope<VirtualMemoryBlock> Create(uint32 size);

		virtual void Clear() = 0;
		virtual void Destroy() = 0;
		virtual uint32 Allocate(uint32 size, uint32 alignment = 0) = 0;
		virtual void Free(uint32 offset) = 0;

		virtual uint32 GetUsedMemorySize() = 0;
		virtual uint32 GetFreeMemorySize() = 0;

	};

}