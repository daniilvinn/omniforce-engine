#pragma once

#include <Foundation/Common.h>

#include <taskflow/taskflow.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Omni {

    class JobSystem;

    class Task {
    public:
        Task() = default;

        bool IsValid() const { return m_State != nullptr; }
        bool IsDone() const { return m_State ? (m_State->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) : true; }
        void Wait() const { if (m_State) m_State->future.wait(); }
        void Reset() { m_State.reset(); }

        // Schedule a continuation that runs after this task
        Task Then(std::function<void()> continuation) const;

        template <typename F>
        auto ThenWithResult(F&& continuation) const -> std::future<std::invoke_result_t<F>>;

        tf::AsyncTask RawHandle() const { return m_State ? m_State->async : tf::AsyncTask{}; }

        // Factory for creating a task from an async handle (used by grouping APIs)
        static Task FromAsync(tf::Executor* executor, tf::AsyncTask async_handle);

    public:
        struct State {
            tf::Executor* executor = nullptr;
            tf::AsyncTask async;
            std::shared_future<void> future;
        };

        explicit Task(std::shared_ptr<State> state) : m_State(std::move(state)) {}

        // Expose for internal subsystems in the engine while retaining simple public API
        std::shared_ptr<State> _GetState() const { return m_State; }

    private:
        friend class JobSystem;
        std::shared_ptr<State> m_State;
    };

    class TaskGroup {
    public:
        TaskGroup() = default;

        void Add(const Task& task) { m_Tasks.push_back(task); }
        void Reserve(size_t n) { m_Tasks.reserve(n); }
        size_t Size() const { return m_Tasks.size(); }
        bool Empty() const { return m_Tasks.empty(); }
        void Clear() { m_Tasks.clear(); }
        std::vector<Task>& _MutableTasks() { return m_Tasks; }

        void Wait() const {
            for (const auto& t : m_Tasks) t.Wait();
        }

        const std::vector<Task>& Tasks() const { return m_Tasks; }

        // Chain a continuation that depends on all tasks in the group
        Task ThenAll(std::function<void()> continuation) const;

    private:
        std::vector<Task> m_Tasks;
    };

    // ---- Inline templates ----
    template <typename F>
    auto Task::ThenWithResult(F&& continuation) const -> std::future<std::invoke_result_t<F>>
    {
        OMNIFORCE_ASSERT(IsValid());
        using R = std::invoke_result_t<F>;
        tf::TaskParams params; // name and priority can be set by higher-level APIs if needed
        auto [dep, fut] = m_State->executor->dependent_async(
            std::move(params),
            std::forward<F>(continuation),
            m_State->async
        );
        (void)dep; // user can obtain the returned Task through JobSystem helpers if needed
        return fut;
    }

} // namespace Omni


