#pragma once

#include <Foundation/Common.h>

#include <Threading/JobSystem.h>

namespace Omni {

	class ObjectLifetimeManager {
	public:
		inline static const uint32 NUM_WAITING_FRAMES = 3;
		inline static const uint32 NUM_DELETION_QUEUES = NUM_WAITING_FRAMES + 1;

		ObjectLifetimeManager()
			: m_CoreObjectDeletionQueue(&g_DedicatedMemoryAllocator)
			, m_DeletionQueues(&g_DedicatedMemoryAllocator)
		{
			for (auto& queue : m_DeletionQueues) {
				queue = Stack<std::function<void()>>(&g_PersistentAllocator);
			}
		}

		// Ticks queues, so in the upcoming frame we will register deletions into a next queue
		void Update() {
			m_CurrentPendingDeletionQueue++;
			m_CurrentPendingDeletionQueue = m_CurrentPendingDeletionQueue % NUM_DELETION_QUEUES;

			m_CurrentDeletionQueue++;
			m_CurrentDeletionQueue = m_CurrentDeletionQueue % NUM_DELETION_QUEUES;
		};

		// CAUTION: must only be called for core objects that must be deleted in a strong order ONLY when engine
		// is shutting down. The best example is Vulkan's instance or device
		void EnqueueCoreObjectDelection(std::function<void()> callable) {
			m_CoreObjectDeletionQueue.Push(callable);
		}

		// Deletes all core objects that were registered by `EnqueueCoreObjectDelection()`, also waits on all
		// async queue executions. Clears all "usual" objects before clearing core objects
		void ExecuteCoreObjectDeletionQueue() {
			OMNIFORCE_ASSERT_TAGGED(!m_CoreObjectQueueExecuted, "This function should only be called once when engine is shutting down");

			JobSystem::WaitForAll();

			FlushDeletionQueues();

			while (!m_CoreObjectDeletionQueue.IsEmpty()) {
				auto callable = m_CoreObjectDeletionQueue.Top();
				callable();
				m_CoreObjectDeletionQueue.Pop();
			}

			m_CoreObjectQueueExecuted = true;
		}

		// Adds an object to the deletion queue. Queue will be executed after 3 frames in async mode.
		void EnqueueObjectDeletion(std::function<void()> deleting_procedure) {
			m_DeletionQueues[m_CurrentDeletionQueue].Push(deleting_procedure);
		}

		// Flushes pending queue in async mode. Also supports sync mode, 
		// but must be used with caution - may lead to performance penalties
		void ExecutePendingDeletionQueue(bool wait = false) {
			if (wait) {
				auto& pending_queue = m_DeletionQueues[m_CurrentPendingDeletionQueue];

				while (!pending_queue.IsEmpty()) {
					auto deletion_procedure = pending_queue.Top();
					deletion_procedure();
					pending_queue.Pop();
				}
			}
			else [[likely]] {
				JobSystem::GetExecutor()->async(
					[pending_queue = m_DeletionQueues[m_CurrentPendingDeletionQueue]]() mutable {
						while (!pending_queue.IsEmpty()) {
							auto deletion_procedure = pending_queue.Top();
							deletion_procedure();
							pending_queue.Pop();
						}
					}
				);
			}

			m_DeletionQueues[m_CurrentPendingDeletionQueue].Clear();
		}
	private:
		// Flushes all queues, should only be called once when engine is shutting down
		void FlushDeletionQueues() {
			for (uint32 i = 0; i < NUM_DELETION_QUEUES; i++) {
				ExecutePendingDeletionQueue(true);
				Update();
			}
		}

	private:
		// A single queue of core objects
		Stack<std::function<void()>> m_CoreObjectDeletionQueue;

		// A queue index that we write to in current frame
		uint32 m_CurrentDeletionQueue = 0;

		// A queue index that must be executed at the end of the frame
		uint32 m_CurrentPendingDeletionQueue = 1;

		// Deletion queues
		StaticArray<Stack<std::function<void()>>, NUM_DELETION_QUEUES> m_DeletionQueues;

#ifdef OMNIFORCE_DEBUG
		bool m_CoreObjectQueueExecuted = false;
#endif

	};

}