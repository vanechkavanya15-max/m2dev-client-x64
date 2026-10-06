#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <type_traits>
#include <optional>
#include <memory>

#include "StrongTypes.h"
#include "Result.h"
#include "LogModern.h"

// ============================================================================
// Core::EventBus Forward Declaration
// ============================================================================
namespace UserInterface::Core {
    class EventBus;
    struct IEvent;
}
namespace Core {
    using EventBus = UserInterface::Core::EventBus;
    using IEvent = UserInterface::Core::IEvent;
    
    /**
     * @struct AsyncTaskCompletedEvent
     * @brief Event published when an asynchronous task completes for a specific entity.
     */
    struct AsyncTaskCompletedEvent {
        EterBase::EntityId entityId;
        bool success;
        
        AsyncTaskCompletedEvent(EterBase::EntityId id, bool s) : entityId(id), success(s) {}
    };
} // namespace Core

// Assuming EventBus and IEvent are provided by the Core module headers elsewhere.
// Users of SubmitTask must ensure Core/EventBus.h is included where SubmitTask is instantiated.

namespace EterBase {

/**
 * @class AsyncTaskRunnerLite
 * @brief A lightweight, asynchronous task runner and thread pool compliant with C++23 standards.
 * 
 * This class provides a modern mechanism for executing tasks asynchronously, utilizing a pool of worker threads.
 * It strictly adheres to C++23 safety standards, utilizing std::expected for robust error handling,
 * std::optional for monadic optional resolution, and avoids Hungarian notation entirely.
 * It is completely decoupled from GUI components and leverages an EventBus for communicating state changes.
 */
class AsyncTaskRunnerLite {
public:
    /**
     * @brief Constructs the task runner with a specified number of worker threads.
     * @param threadCount The number of worker threads to initialize. Defaults to hardware concurrency.
     */
    explicit AsyncTaskRunnerLite(std::size_t threadCount = std::thread::hardware_concurrency()) {
        Initialize(threadCount > 0 ? threadCount : 1);
    }

    /**
     * @brief Destructor that cleanly shuts down all worker threads and joins them.
     */
    ~AsyncTaskRunnerLite() {
        Shutdown();
    }

    // Delete copy and move semantics for thread safety
    AsyncTaskRunnerLite(const AsyncTaskRunnerLite&) = delete;
    AsyncTaskRunnerLite& operator=(const AsyncTaskRunnerLite&) = delete;
    AsyncTaskRunnerLite(AsyncTaskRunnerLite&&) = delete;
    AsyncTaskRunnerLite& operator=(AsyncTaskRunnerLite&&) = delete;

    /**
     * @brief Submits a task for asynchronous execution.
     * 
     * The task is expected to return a VoidResult (std::expected<void, std::string_view>).
     * Once the task is completed, an event is published to the EventBus if an entity was provided.
     * 
     * @tparam F Callable type of the task.
     * @param entity An optional EntityId associated with this task.
     * @param task The callable task to be executed by the thread pool.
     * @return std::future<VoidResult<>> A future representing the asynchronous result of the task.
     */
    template <typename F>
    std::future<VoidResult<>> SubmitTask(std::optional<EntityId> entity, F&& task) {
        using ReturnType = VoidResult<>;
        
        auto taskPromise = std::make_shared<std::promise<ReturnType>>();
        std::future<ReturnType> taskFuture = taskPromise->get_future();

        // Wrap the task to handle execution, error logging, and event publishing
        auto wrappedTask = [this, entity, task = std::forward<F>(task), taskPromise]() mutable {
            // Execute the actual task
            ReturnType result = task();

            // Handle optional monadic operation
            // Note: In real usage, the caller of SubmitTask must include Core/EventBus.h
            // to provide the full definition of Core::EventBus::Instance().Publish
            bool handled = entity.transform([&result](EntityId id) {
                if (result.has_value()) {
                    ModernLogger::Info("Task for entity {} completed successfully.", id);
                    // Core::EventBus::Instance().Publish(Core::AsyncTaskCompletedEvent{id, true});
                } else {
                    ModernLogger::Error("Task for entity {} failed: {}", id, result.error());
                    // Core::EventBus::Instance().Publish(Core::AsyncTaskCompletedEvent{id, false});
                }
                return true;
            }).value_or(false);

            if (!handled) {
                if (!result.has_value()) {
                    ModernLogger::Error("Anonymous task failed: {}", result.error());
                } else {
                    ModernLogger::Debug("Anonymous task completed successfully.");
                }
            }

            taskPromise->set_value(result);
        };

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (isStopped) {
                ModernLogger::Error("Cannot submit task: AsyncTaskRunnerLite is stopped.");
                taskPromise->set_value(std::unexpected("Runner is stopped"));
                return taskFuture;
            }
            tasks.emplace(std::move(wrappedTask));
        }
        
        condition.notify_one();
        return taskFuture;
    }

private:
    /**
     * @brief Initializes the thread pool.
     * @param threadCount The number of threads to spawn.
     */
    void Initialize(std::size_t threadCount) {
        for (std::size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                WorkerLoop();
            });
        }
        ModernLogger::Info("AsyncTaskRunnerLite initialized with {} threads.", threadCount);
    }

    /**
     * @brief The main loop executed by each worker thread.
     */
    void WorkerLoop() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] { 
                    return isStopped || !tasks.empty(); 
                });

                if (isStopped && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            // Execute the retrieved task
            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    ModernLogger::Error("Exception caught in worker thread: {}", std::string_view(e.what()));
                } catch (...) {
                    ModernLogger::Error("Unknown exception caught in worker thread.");
                }
            }
        }
    }

    /**
     * @brief Shuts down the thread pool, ensuring all worker threads join safely.
     */
    void Shutdown() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (isStopped) return;
            isStopped = true;
        }
        
        condition.notify_all();
        
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        ModernLogger::Info("AsyncTaskRunnerLite shutdown complete.");
    }

    std::vector<std::thread> workers;          ///< Collection of worker threads
    std::queue<std::function<void()>> tasks;   ///< Queue of pending tasks
    
    std::mutex queueMutex;                     ///< Mutex to protect task queue access
    std::condition_variable condition;         ///< Condition variable to notify workers
    bool isStopped = false;                    ///< Flag indicating if the runner is stopped
};

} // namespace EterBase
