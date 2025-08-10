#include <Threading/JobSystem.h>
#include <Threading/Task.h>

#include <Windows.h>
#include <array>
#include <thread>

namespace Omni {

// Returns the number of worker threads to use for a given priority queue.
// If the requested value is zero, use the hardware concurrency as a default.
static size_t choose_worker_count(uint32 requested) {
    if (requested == 0) {
        return std::thread::hardware_concurrency();
    }

    return requested;
}

// Initializes executors for all priority queues according to the provided config.
// This function is thread-safe and can be called multiple times (no-op after the first).
void JobSystem::Init(const JobSystemConfig& config) {
    if (s_Initialized.load(std::memory_order_acquire)) {
        return;
    }

    s_Config = config;

    const size_t hp = choose_worker_count(config.high_priority_workers);
    const size_t np = choose_worker_count(config.normal_priority_workers);
    const size_t lp = choose_worker_count(config.low_priority_workers);

    s_Executors[static_cast<size_t>(Queue::High)]   = std::make_unique<tf::Executor>(hp);
    s_Executors[static_cast<size_t>(Queue::Normal)] = std::make_unique<tf::Executor>(np);
    s_Executors[static_cast<size_t>(Queue::Low)]    = std::make_unique<tf::Executor>(lp);

    s_Initialized.store(true, std::memory_order_release);
}

// Shuts down all executors and releases associated resources.
void JobSystem::Shutdown() {
    if (!s_Initialized.load(std::memory_order_acquire)) {
        return;
    }

    for (auto& e : s_Executors) {
        e.reset();
    }

    s_Initialized.store(false, std::memory_order_release);
}

// Ensures the system is initialized with default configuration if not already.
void JobSystem::EnsureInitialized() {
    if (!s_Initialized.load(std::memory_order_acquire)) {
        Init({});
    }
}

// Blocks the calling thread until all work submitted to all queues completes.
void JobSystem::WaitForAll() {
    EnsureInitialized();

    for (auto& e : s_Executors) {
        if (e) {
            e->wait_for_all();
        }
    }
}

// Submits a single callable to the specified queue and returns a Task handle.
Task JobSystem::Submit(std::function<void()> work, const TaskDesc& desc, Queue queue) {
    EnsureInitialized();

    const size_t qidx = static_cast<size_t>(queue);
    auto* exec = s_Executors[qidx].get();

    tf::TaskParams params;
    if (desc.name) {
        params.name = desc.name;
    }

    // Map priority to integer (High:2, Normal:1, Low:0) → higher numbers = higher priority for TF.
    switch (desc.priority) {
        case TaskPriority::High: {
            params.priority = 2;
            break;
        }
        case TaskPriority::Normal: {
            params.priority = 1;
            break;
        }
        case TaskPriority::Low: {
            params.priority = 0;
            break;
        }
    }

    // Create a root async task with a handle by using zero-length dependency range.
    std::array<tf::AsyncTask, 0> empty{};
    auto dep = exec->silent_dependent_async(params, std::move(work), empty.begin(), empty.end());
    auto [handle, fut] = exec->dependent_async([](){} , dep);

    auto state = std::make_shared<Task::State>();
    state->executor = exec;
    state->async = dep;
    state->future = fut.share();
    return Task(std::move(state));
}

// Submits a batch of callables and returns a TaskGroup that can be waited on.
TaskGroup JobSystem::Submit(std::vector<std::function<void()>> works, const TaskDesc& desc, Queue queue) {
    EnsureInitialized();

    TaskGroup group;
    group.Clear();
    group.Reserve(works.size());

    for (auto& w : works) {
        group.Add(Submit(std::move(w), desc, queue));
    }

    return group;
}

void JobSystem::SetCurrentThreadName(const std::wstring& name) {
    // Windows 10 Creators Update and later.
    HRESULT hr = SetThreadDescription(GetCurrentThread(), name.c_str());
    (void)hr;
}

uint32 JobSystem::GetWorkerCount(Queue queue) {
    EnsureInitialized();
    auto* exec = s_Executors[static_cast<size_t>(queue)].get();
    return static_cast<uint32>(std::max<size_t>(1, exec ? exec->num_workers() : 0));
}

} // namespace Omni


