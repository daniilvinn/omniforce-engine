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
		static tf::Executor* GetExecutor() { return &m_Executor; };

	private:
		inline static tf::Executor m_Executor;

	};
}