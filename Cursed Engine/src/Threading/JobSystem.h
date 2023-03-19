#pragma once

#include <Foundation/Types.h>

#include <thread>
#include <queue>
#include <functional>
#include <shared_mutex>
#include <assert.h>
#include <atomic>

namespace Cursed {

	enum class JobPriority : uint32 {
		STANDARD,
		HIGH
	};

	struct JobDeclaration {
		std::function<void()> job;
		std::atomic_bool finished = false;
		JobDeclaration* dependencies = nullptr;
		std::atomic<uint32> dependency_counter = 0;
		JobPriority priority = JobPriority::STANDARD;
	};

	enum class JobSystemShutdownPolicy : uint8_t {
		WAIT, // wait on all jobs to be executed before shutting down job system
		CLEAR // clear the queue, so shutdown immediately 
	};

	class JobSystem {
	public:
		
		static void Init(JobSystemShutdownPolicy policy = JobSystemShutdownPolicy::CLEAR);
		static void Destroy() { delete s_Instance; };
		static JobSystem* Get() { return s_Instance; }

		void Execute(std::function<void()> func);
		inline void Wait() const { while (IsBusy()); }
		inline bool IsBusy() const { return !(m_CurrentExecutionLabel == m_FinishExecutionLabel); }
		void ClearQueue();

	private:
		JobSystem(JobSystemShutdownPolicy policy = JobSystemShutdownPolicy::CLEAR);
		~JobSystem();

		static JobSystem* s_Instance;

		std::vector<std::thread> m_ThreadPool;
		std::queue<std::function<void()>> m_Queue;
		std::atomic_bool m_Running;
		uint64 m_CurrentExecutionLabel = 0;
		std::atomic<uint64> m_FinishExecutionLabel = 0;
		std::shared_mutex m_CaptureMutex;
		std::condition_variable_any m_ConditionVar;
		JobSystemShutdownPolicy m_ShutdownPolicy;
	};
}