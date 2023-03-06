#pragma once

#include <Foundation/Macros.h>

#include <shared_mutex>

namespace Cursed {

	template<bool Lock>
	class CURSED_API ConditionalLock;
	
	template<>
	class CURSED_API ConditionalLock<true> {
	public:
		ConditionalLock(){ m_Mutex.lock(); }
		~ConditionalLock() { m_Mutex.unlock(); }

	private:
		std::shared_mutex m_Mutex;
	};

	template<>
	class CURSED_API ConditionalLock<false> 
	{};

}