#pragma once

#include <Foundation/Macros.h>

#include <shared_mutex>

namespace Omni {

	template<bool Lock>
	class OMNIFORCE_API ConditionalLock;
	
	template<>
	class OMNIFORCE_API ConditionalLock<true> {
	public:
		ConditionalLock(){ m_Mutex.lock(); }
		~ConditionalLock() { m_Mutex.unlock(); }

	private:
		std::shared_mutex m_Mutex;
	};

	template<>
	class OMNIFORCE_API ConditionalLock<false> 
	{};

}