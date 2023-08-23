#pragma once

#include <Foundation/Macros.h>

#include <shared_mutex>

namespace Omni {

	template<bool Lock>
	class OMNIFORCE_API ConditionalLock;
	
	template<>
	class OMNIFORCE_API ConditionalLock<true> {
	public:
		void Lock() { m_Mutex.lock(); };
		void Unlock() { m_Mutex.unlock(); };

	private:
		std::shared_mutex m_Mutex;
	};

	template<>
	class OMNIFORCE_API ConditionalLock<false> 
	{
	public:
		void Lock() {}
		void Unlock() {}
	};

}