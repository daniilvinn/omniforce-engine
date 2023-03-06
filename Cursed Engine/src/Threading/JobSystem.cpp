#include "JobSystem.h"

namespace Cursed 
{

	JobSystem* JobSystem::s_Instance;

	void JobSystem::Init(ShutdownPolicy policy)
	{
		s_Instance = new JobSystem(policy);
	}

	JobSystem::JobSystem(ShutdownPolicy policy)
		: m_ShutdownPolicy(policy)
	{
		m_Running = true;

		uint8_t num_threads = std::thread::hardware_concurrency() - 1;
		m_ThreadPool.resize(num_threads);

		for (int i = 0; i < num_threads; i++) {
			m_ThreadPool[i] = std::thread([&]() {
				while (m_Running) {
					std::unique_lock<std::shared_mutex> lock(m_CaptureMutex);
					while (m_Queue.empty()) {
						if (!m_Running)
							return;
						m_ConditionVar.wait(lock);
					}

					auto job = m_Queue.front();
					m_Queue.pop();
					lock.unlock();

					job();
					m_FinishExecutionLabel += 1;
				}
			});
		}
	}

	JobSystem::~JobSystem() {

		if (m_ShutdownPolicy == ShutdownPolicy::WAIT) {
			while (!m_Queue.empty());
		}
		else {
			ClearQueue();
		}

		m_Running = false;
		m_ConditionVar.notify_all();

		for (int i = 0; i < m_ThreadPool.size(); i++) {
			m_ThreadPool[i].join();
		}
	}

	void JobSystem::Execute(std::function<void()> func)
	{
		m_CurrentExecutionLabel += 1;

		std::scoped_lock lock(m_CaptureMutex);
		m_Queue.push(func);
		m_ConditionVar.notify_one();
	}

	void JobSystem::ClearQueue()
	{
		std::scoped_lock lock(m_CaptureMutex);
		while (!m_Queue.empty())
			m_Queue.pop();
	}

}