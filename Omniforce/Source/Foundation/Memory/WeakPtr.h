#pragma once

#include <Foundation/Platform.h>
#include <Foundation/BasicTypes.h>

#include <Foundation/Memory/PtrCommon.h>
#include <Foundation/Memory/MemoryAllocation.h>
#include <Foundation/Memory/Ptr.h>
#include <Foundation/Memory/Ref.h>
#include <Foundation/Assert.h>

namespace Omni {
	template<typename T>
	class Ref;

	template<typename T>
	class Ptr;

	template<typename T>
	class WeakPtr;

	template<template<typename> class PtrType, typename T, typename U>
	concept SupportsWeakPtr = ((std::is_same_v<PtrType<U>, Ref<U>> || std::is_same_v<PtrType<U>, Ptr<U>> || std::is_same_v<PtrType<U>, WeakPtr<U>>) && PtrCheckType<T, U>);

	template<template<typename> typename PtrType, typename T>
	concept IsPtrOrRef = (std::is_same_v<PtrType<T>, Ptr<T>> || std::is_same_v<PtrType<T>, Ref<T>>);

	/*
	*  @brief A pointer that only holds a weak reference to an object, e.g. it doesn't
	*  own the object so it cannot perform object deallocation
	*/
	template<typename T>
	class OMNIFORCE_API WeakPtr {
	private:
		template<typename U>
		WeakPtr(T* object)
			: m_Object(object)
		{}

		WeakPtr(T* object)
			: m_Object(object)
		{}

	public:
		WeakPtr(MemoryAllocation allocation)
			: m_Object(allocation.As<T>())
		{}

		WeakPtr(const WeakPtr& other) {
			m_Object = other.Raw();
		}

		WeakPtr(WeakPtr&& other) {
			m_Object = other.Raw();
			other.m_Object = nullptr;
		}

		// Allow copy constructors
		template<template<typename> class PtrType, typename U>
		requires SupportsWeakPtr<PtrType, T, U>
		WeakPtr<T>(const PtrType<U>& other) noexcept
		{
			T* m_Object = (T*)other.Raw();
		}

		// Forbid move constructors
		template<template<typename> class PtrType, typename U>
		requires IsPtrOrRef<PtrType, U>
		WeakPtr(PtrType<U>&& other) noexcept = delete;

		// Allow copy assignment
		template<template<typename> class PtrType, typename U>
		requires SupportsWeakPtr<PtrType, T, U>
		WeakPtr<T>& operator=(const PtrType<U>& other) noexcept
		{
			T* m_Object = (T*)other.Raw();
		}

		// Forbid move assignment
		template<template<typename> class PtrType, typename U>
		requires IsPtrOrRef<PtrType, U>
		WeakPtr<T>& operator=(PtrType<U>&& other) noexcept = delete;

		WeakPtr& operator=(const WeakPtr& other) noexcept 
		{
			m_Object = other.Raw();
			return *this;
		}

		WeakPtr& operator=(WeakPtr&& other) noexcept
		{
			m_Object = other.Raw();
			other.m_Object = nullptr;
			*this;
		}

		inline T* operator->() const {
			return m_Object;
		}

		inline T* Raw() const {
			return m_Object;
		}

		template<typename U>
		inline WeakPtr<U> As() const {
			return WeakPtr<U>(m_Object);
		}

		inline operator bool() const {
			return m_Object != nullptr;
		}

	private:
		

	private:
		T* m_Object;
	};

}