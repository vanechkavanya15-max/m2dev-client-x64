#pragma once

#include <future>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cstdint>

namespace Client::Concurrency {

/**
 * @brief A lightweight, asynchronous task execution framework based on a work-stealing thread pool.
 * 
 * JobSystem allows submission of arbitrary invocables (functions, lambdas, functors) for concurrent
 * execution, returning a std::future for retrieving results or waiting for completion. It features
 * work stealing to automatically balance load across worker threads.
 */
class JobSystem {
public:
    using Job = std::function<void()>;

    /**
     * @brief Constructs the JobSystem with a specific number of threads.
     * @param numThreads The number of worker threads to spawn. If 0, uses hardware concurrency.
     */
    explicit JobSystem(std::size_t numThreads = 0);

    /**
     * @brief Destructor. Signals all workers to stop and waits for them to finish current jobs.
     */
    ~JobSystem() noexcept;

    // Non-copyable and non-movable due to internal synchronization primitives and threading
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    /**
     * @brief Submits a task for asynchronous execution.
     * 
     * @tparam F The type of the invocable.
     * @tparam Args The types of the arguments.
     * @param f The invocable object to execute.
     * @param args The arguments to pass to the invocable.
     * @return A std::future representing the eventual result of the task.
     */
    template<typename F, typename... Args>
    [[nodiscard]] auto Submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
    {
        using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable -> ReturnType {
                return std::invoke(std::move(func), std::move(args)...);
            }
        );

        auto res = task->get_future();
        
        Job job = [task]() { (*task)(); };

        PushJob(std::move(job));

        return res;
    }

    /**
     * @brief Blocks the calling thread until all currently queued and active jobs have completed.
     */
    void WaitAll();

private:
    void PushJob(Job job);
    void WorkerThread(std::size_t workerId);
    bool TrySteal(Job& job, std::size_t workerId);

    std::vector<std::jthread> m_threads;
    std::vector<std::deque<Job>> m_queues;
    std::unique_ptr<std::mutex[]> m_mutexes;
    
    std::atomic<bool> m_stop{false};
    std::atomic<std::size_t> m_activeJobs{0};
    std::atomic<std::size_t> m_queuedJobs{0};
    
    std::condition_variable m_cv;
    std::mutex m_cvMutex;
    
    std::condition_variable m_waitCv;
    std::mutex m_waitMutex;
};

} // namespace Client::Concurrency
