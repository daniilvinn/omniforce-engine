#pragma once

#include "MemoryAllocation.h"
#include "Memory/Allocator.h"

#include <memory>
#include <iostream>

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

	// A pointer that only holds a reference to an object,
	// but doesn't perform deallocation once it is out of the scope
	template<typename T>
	class OMNIFORCE_API WeakPtr {
	public:
		WeakPtr(MemoryAllocation InAllocation)
		: mAllocation(InAllocation) {}

		inline T* operator->() const {
			return mAllocation.As<T*>();
		}

		inline const T* Raw() const {
			return (const T*)mAllocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(mAllocation);
		}

		inline operator bool() const {
			return mAllocation.IsValid();
		}

	private:
		MemoryAllocation mAllocation;
	};

	// Smart pointer that performs auto-deletion of an object
	// once pointer is out of the scope and destructor is invoked
	template<typename T>
	class OMNIFORCE_API Ptr {

		template<typename... TArgs>
		Ptr(IAllocator& InAllocator, TArgs&&... InArgs)
			: mAllocator(InAllocator) {
			mAllocation = mAllocator.Allocate<T>(std::forward<TArgs>(InArgs)...);
		};

	public:
		~Ptr() {
			mAllocator.Free(mAllocation);
		}

		template <typename TAlloc, typename... TArgs>
		static auto Create(TAlloc& allocator, TArgs&&... args) {
			return Ptr<T>(allocator, std::forward<TArgs>(args)...);
		}

		Ptr(const Ptr& InOther) = delete;
		Ptr(Ptr&& InOther) noexcept
			: mAllocator(InOther.mAllocator)
			, mAllocation(InOther.mAllocation)
		{
			InOther.mAllocation.Invalidate();
		}

		Ptr<T>& operator=(const Ptr<T>& InOther) = delete;
		Ptr<T>& operator=(Ptr<T>&& InOther) noexcept {
			if (mAllocation.IsValid())
				mAllocator->Free(mAllocation);

			mAllocator = InOther.mAllocator;
			mAllocation = InOther.mAllocation;

			InOther.mAllocation.Invalidate();

			return *this;
		};

		T* operator->() {
			return mAllocation.As<T>();
		}

		inline T& operator*() {
			return *mAllocation.As<T>();
		}

		inline const T* Raw() const {
			return (const T*)mAllocation.Memory;
		}

		template<typename NewType>
		inline WeakPtr<NewType> As() const {
			return WeakPtr<NewType>(mAllocation);
		}

		inline operator bool() const { 
			return mAllocation.IsValid(); 
		}

	private:
		MemoryAllocation mAllocation;
		IAllocator& mAllocator;

	};

}