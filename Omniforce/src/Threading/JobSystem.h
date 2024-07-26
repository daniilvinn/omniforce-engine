#pragma once

#include <Foundation/Types.h>

#include <thread>
#include <queue>
#include <functional>
#include <shared_mutex>
#include <assert.h>
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