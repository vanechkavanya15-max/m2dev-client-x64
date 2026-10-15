#pragma once

#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include <expected>
#include <optional>
#include <span>

namespace Client::Concurrency {

    enum class DispatcherError {
        QueueEmpty,
        TaskExecutionFailed,
        BudgetExceeded
    };

    template <typename T, typename E = DispatcherError>
    using Result = std::expected<T, E>;

    class MainThreadDispatcher final {
    public:
        using Task = std::function<void()>;
        using Clock = std::chrono::steady_clock;
        using Duration = std::chrono::nanoseconds;

        MainThreadDispatcher() noexcept = default;
        ~MainThreadDispatcher() noexcept = default;

        MainThreadDispatcher(const MainThreadDispatcher&) = delete;
        MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;
        MainThreadDispatcher(MainThreadDispatcher&&) = delete;
        MainThreadDispatcher& operator=(MainThreadDispatcher&&) = delete;

        void Post(Task task) {
            if (!task) return;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(task));
        }

        void PostBatch(std::span<const Task> tasks) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& task : tasks) {
                if (task) {
                    m_queue.push(task);
                }
            }
        }

        Result<size_t> Dispatch(Duration timeBudget = std::chrono::milliseconds(2)) {
            auto startTime = Clock::now();
            size_t executedCount = 0;
            bool budgetExceeded = false;

            while (true) {
                auto now = Clock::now();
                if (now - startTime >= timeBudget) {
                    budgetExceeded = true;
                    break;
                }

                std::optional<Task> task = PopTask();
                if (!task.has_value()) {
                    break;
                }

                try {
                    (*task)();
                    executedCount++;
                } catch (...) {
                    executedCount++;
                    // Zabezpieczenie przed przerwaniem dispatchowania przez wyjatek
                }
            }

            if (executedCount == 0) {
                if (budgetExceeded && GetPendingCount() > 0) {
                    return std::unexpected(DispatcherError::BudgetExceeded);
                }
                if (GetPendingCount() == 0) {
                    return std::unexpected(DispatcherError::QueueEmpty);
                }
            }

            return executedCount;
        }

        [[nodiscard]] size_t GetPendingCount() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        void Clear() {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::queue<Task> empty;
            std::swap(m_queue, empty);
        }

    private:
        std::optional<Task> PopTask() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return std::nullopt;
            }
            Task task = std::move(m_queue.front());
            m_queue.pop();
            return task;
        }

        mutable std::mutex m_mutex;
        std::queue<Task> m_queue;
    };

} // namespace Client::Concurrency
