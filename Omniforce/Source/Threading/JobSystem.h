#pragma once

#include <Foundation/Common.h>

#include <thread>
#include <queue>
#include <shared_mutex>
#include <atomic>

#include <taskflow/taskflow.hpp>

namespace Omni {

	class JobSystem {
	public:
		static tf::Executor* GetExecutor() { 
			return &m_Executor; 
		};

		static void WaitForAll() { 
			return m_Executor.wait_for_all(); 
		}

	private:
		inline static tf::Executor m_Executor;

	};
}