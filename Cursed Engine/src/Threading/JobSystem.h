#pragma once

#include <Foundation/Types.h>

#include <thread>
#include <queue>
#include <functional>
#include <shared_mutex>
#include <assert.h>
#include <atomic>

namespace Cursed {

	class JobSystem {
	public:

		struct Declaration {
			std::function<void()> job;
			std::atomic_bool finished = false;
			Declaration* dependencies = nullptr;
		};

		enum class ShutdownPolicy : uint8_t {
			WAIT, // wait on all jobs to be executed before shutting down job system
			CLEAR // clear the queue, so shutdown immediately 
		};
		
		static void Init(ShutdownPolicy policy = ShutdownPolicy::CLEAR);
		static void Destroy() { delete s_Instance; };
		static JobSystem* Get() { return s_Instance; }

		void Execute(std::function<void()> func);

		inline bool IsBusy() const { return !(m_CurrentExecutionLabel == m_FinishExecutionLabel); }

		inline void Wait() const { while (IsBusy()); }

		void ClearQueue();

	private:
		JobSystem(ShutdownPolicy policy = ShutdownPolicy::CLEAR);
		~JobSystem();

		static JobSystem* s_Instance;

		std::vector<std::thread> m_ThreadPool;
		std::queue<std::function<void()>> m_Queue;
		std::atomic_bool m_Running;
		uint64 m_CurrentExecutionLabel = 0;
		std::atomic<uint64> m_FinishExecutionLabel = 0;
		std::shared_mutex m_CaptureMutex;
		std::condition_variable_any m_ConditionVar;
		ShutdownPolicy m_ShutdownPolicy;
	};
}