#pragma once

#include "MemoryAllocation.h"
#include "Allocator.h"

#include <memory>

namespace Omni {

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T>
	using Shared = std::shared_ptr<T>;

	template <typename T1, typename T2>
	Shared<T1> ShareAs(Shared<T2>& ptr)
	{
		return std::static_pointer_cast<T1>(ptr);
	};
	
	template<typename T>
	class OMNIFORCE_API Ptr {
	public:
		template<typename... Args>
		Ptr(IAllocator& Allocator, Args&&... args)
			: mAllocator(Allocator) {
			Allocator.Allocate<T>(std::forward<Args>(args));
		};

		~Ptr() {
			mAllocator.Free(mAllocation);
		}

		Ptr(const Ptr& other) = delete;
		Ptr(Ptr&& other) = delete;

		T* operator->() const {
			return mAllocation.As<T>();
		}

	private:
		MemoryAllocation mAllocation;
		IAllocator& mAllocator;
	};

}