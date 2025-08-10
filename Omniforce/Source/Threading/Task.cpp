#include <Threading/Task.h>
#include <Threading/JobSystem.h>

namespace Omni {

Task Task::Then(std::function<void()> continuation) const {
    // A continuation can only be attached to a valid task.
    OMNIFORCE_ASSERT(IsValid());

    tf::TaskParams params;
    auto dep = m_State->executor->silent_dependent_async(
        std::move(params),
        std::move(continuation),
        m_State->async
    );

    auto state = std::make_shared<State>();
    state->executor = m_State->executor;
    state->async = dep;

    // We still want a future to wait on; create a trivial one via dependent_async that returns void.
    auto [task_handle, fut] = m_State->executor->dependent_async([](){} , dep);
    (void)task_handle; // Future ties the completion of dep.
    state->future = fut.share();
    return Task(std::move(state));
}

Task TaskGroup::ThenAll(std::function<void()> continuation) const {
    // The group must have at least one task to form a dependency set.
    OMNIFORCE_ASSERT(!m_Tasks.empty());

    // Use the first task's executor (all tasks in a group are submitted through JobSystem and share exec).
    tf::Executor* executor = m_Tasks.front()._GetState()->executor;

    // Collect dependencies from tasks in the group.
    std::vector<tf::AsyncTask> deps;
    deps.reserve(m_Tasks.size());
    for (const auto& t : m_Tasks) {
        deps.emplace_back(t.RawHandle());
    }

    tf::TaskParams params;
    auto dep = executor->silent_dependent_async(
        std::move(params),
        std::move(continuation),
        deps.begin(),
        deps.end()
    );

    auto state = std::make_shared<Task::State>();
    state->executor = executor;
    state->async = dep;
    auto [task_handle, fut] = executor->dependent_async([](){} , dep);
    (void)task_handle;
    state->future = fut.share();
    return Task(std::move(state));
}

Task Task::FromAsync(tf::Executor* executor, tf::AsyncTask async_handle) {
    // Wrap a raw async handle into a Task with a waitable future.
    auto state = std::make_shared<State>();
    state->executor = executor;
    state->async = async_handle;

    // Create a trivial dependent future to enable waiting.
    auto [task_handle, fut] = executor->dependent_async([](){} , state->async);
    (void)task_handle;
    state->future = fut.share();
    return Task(std::move(state));
}

} // namespace Omni


