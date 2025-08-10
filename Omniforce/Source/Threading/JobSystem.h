#pragma once

#include <Foundation/Common.h>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <taskflow/taskflow.hpp>

#include <Threading/Task.h>

namespace Omni {

    enum class TaskPriority : uint8 {
        High = 0,
        Normal = 1,
        Low = 2,
    };

    struct TaskDesc {
        const char* name = nullptr;         // Optional human-readable name
        const char* category = "General";   // Optional category/tag
        TaskPriority priority = TaskPriority::Normal;
    };

    struct JobSystemConfig {
        // Number of worker threads per priority queue. 0 -> use hardware_concurrency
        uint32 high_priority_workers = 0;
        uint32 normal_priority_workers = 0;
        uint32 low_priority_workers = 0;
        // Allow running small continuations inline on calling thread
        bool allow_inline_continuations = true;
    };

    class JobSystem {
    public:
        enum class Queue : uint8 { High = 0, Normal = 1, Low = 2 };

        // Initialize / shutdown
        static void Init(const JobSystemConfig& config = {});
        static void Shutdown();

        // Backwards-compatible accessor
        static tf::Executor* GetExecutor() { EnsureInitialized(); return s_Executors[static_cast<size_t>(Queue::Normal)].get(); }
        static tf::Executor* GetExecutor(Queue queue) { EnsureInitialized(); return s_Executors[static_cast<size_t>(queue)].get(); }

        static void WaitForAll();

        // Submit a fire-and-forget task (returns a Task that can be waited on)
        static Task Submit(std::function<void()> work, const TaskDesc& desc = {}, Queue queue = Queue::Normal);

        // Submit many tasks at once, returns a group for collective waiting
        static TaskGroup Submit(std::vector<std::function<void()>> works, const TaskDesc& desc = {}, Queue queue = Queue::Normal);

        // Submit a callable that returns a value, returns a std::future<R>
        template <typename F>
        static auto SubmitWithResult(F&& callable, const TaskDesc& desc = {}, Queue queue = Queue::Normal)
            -> std::future<std::invoke_result_t<F>>
        {
            using R = std::invoke_result_t<F>;
            auto promise_ptr = std::make_shared<std::promise<R>>();
            auto fut = promise_ptr->get_future();
            auto wrapper = [promise_ptr, c = std::forward<F>(callable)]() mutable {
                try {
                    if constexpr (std::is_void_v<R>) {
                        c();
                        promise_ptr->set_value();
                    } else {
                        promise_ptr->set_value(c());
                    }
                } catch (...) {
                    promise_ptr->set_exception(std::current_exception());
                }
            };
            GetExecutor(queue)->async(std::move(wrapper));
            return fut;
        }

        // Parallel-for helpers (asynchronous). Returns a TaskGroup that can be waited on.
        template <typename Index, typename Func>
        static TaskGroup ParallelFor(Index begin, Index end, Index grain_size, Func fn, const TaskDesc& desc = {}, Queue queue = Queue::Normal);

        template <typename Iter, typename Func>
        static TaskGroup ParallelForEach(Iter begin, Iter end, size_t grain_size, Func fn, const TaskDesc& desc = {}, Queue queue = Queue::Normal);

        // Utilities
        static void SetCurrentThreadName(const std::wstring& name);
        static uint32 GetWorkerCount(Queue queue);

    private:
        static void EnsureInitialized();

        inline static std::unique_ptr<tf::Executor> s_Executors[3];
        inline static JobSystemConfig s_Config{};
        inline static std::atomic<bool> s_Initialized = false;
    };

    // ---- Inline template implementations ----
    template <typename Index, typename Func>
    TaskGroup JobSystem::ParallelFor(Index begin, Index end, Index grain_size, Func fn, const TaskDesc& desc, Queue queue)
    {
        OMNIFORCE_ASSERT(begin <= end);
        OMNIFORCE_ASSERT(grain_size > 0);
        const Index total = end - begin;
        const Index chunk = static_cast<Index>(grain_size);
        const Index num_chunks = (total + chunk - 1) / chunk;

        std::vector<std::function<void()>> works;
        works.reserve(static_cast<size_t>(num_chunks));
        for (Index i = 0; i < num_chunks; ++i) {
            Index s = begin + i * chunk;
            Index e = std::min(end, s + chunk);
            works.emplace_back([s, e, fn]() {
                for (Index j = s; j < e; ++j) fn(j);
            });
        }
        return Submit(std::move(works), desc, queue);
    }

    template <typename Iter, typename Func>
    TaskGroup JobSystem::ParallelForEach(Iter begin, Iter end, size_t grain_size, Func fn, const TaskDesc& desc, Queue queue)
    {
        OMNIFORCE_ASSERT(grain_size > 0);
        const size_t total = static_cast<size_t>(std::distance(begin, end));
        const size_t chunk = grain_size;
        const size_t num_chunks = (total + chunk - 1) / chunk;

        std::vector<std::function<void()>> works;
        works.reserve(num_chunks);
        for (size_t i = 0; i < num_chunks; ++i) {
            size_t s_idx = i * chunk;
            size_t e_idx = std::min(total, s_idx + chunk);
            works.emplace_back([begin, s_idx, e_idx, fn]() {
                auto it = std::next(begin, static_cast<std::ptrdiff_t>(s_idx));
                for (size_t j = s_idx; j < e_idx; ++j, ++it) fn(*it);
            });
        }
        return Submit(std::move(works), desc, queue);
    }
}
