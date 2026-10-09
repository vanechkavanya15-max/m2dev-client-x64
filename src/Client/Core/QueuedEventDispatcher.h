#pragma once

#include <vector>
#include <mutex>
#include <functional>
#include <utility>
#include <expected>
#include <cstdint>
#include <string_view>

#include "Result.h"
#include "EventBus.h"

namespace Client::Core {

/**
 * @brief Enumy bledow domenowych operacji dispatchera.
 */
enum class DispatcherError : uint8_t {
    None = 0,
    QueueFull,
    EmptyTask
};

[[nodiscard]] inline std::string_view to_string(DispatcherError error) noexcept {
    switch (error) {
        case DispatcherError::None: return "DispatcherError::None";
        case DispatcherError::QueueFull: return "DispatcherError::QueueFull - Cannot enqueue, queue is full";
        case DispatcherError::EmptyTask: return "DispatcherError::EmptyTask - Cannot enqueue an empty task";
        default: return "DispatcherError::Unknown";
    }
}

/**
 * @brief Komponent odpowiedzialny za gromadzenie zadan z watkow pobocznych i ich wykonanie na glownym ticku.
 * Zgodny ze standardem AI-First Architecture. (C++20/23).
 */
class QueuedEventDispatcher {
public:
    using Task = std::function<void()>;
    
    // Typ zwaracany wykorzystuje Result<T, E> z Result.h
    using ResultType = std::expected<void, DispatcherError>;

    static constexpr size_t DEFAULT_MAX_QUEUE_SIZE = 10000;

    QueuedEventDispatcher(const QueuedEventDispatcher&) = delete;
    QueuedEventDispatcher& operator=(const QueuedEventDispatcher&) = delete;
    QueuedEventDispatcher(QueuedEventDispatcher&&) = delete;
    QueuedEventDispatcher& operator=(QueuedEventDispatcher&&) = delete;

    /**
     * @brief Zwraca globalna instancje.
     */
    static QueuedEventDispatcher& Instance() {
        static QueuedEventDispatcher instance;
        return instance;
    }

    /**
     * @brief Zwraca globalna instancje (alias).
     */
    static QueuedEventDispatcher& GetInstance() {
        return Instance();
    }

    /**
     * @brief Dodaje zdarzenie do kolejki asynchronicznie i deleguje jego publikacje na glowny watek do EventBusa.
     */
    template <typename EventType>
    ResultType EnqueueEvent(EventType event) {
        auto task = [evt = std::move(event)]() {
            ::Client::Core::EventBus::Instance().Publish(evt);
        };
        return EnqueueTask(std::move(task));
    }

    /**
     * @brief Dodaje dowolne zadanie do asynchronicznej kolejki na glownym watku.
     */
    ResultType EnqueueTask(Task task) {
        if (!task) {
            return std::unexpected(DispatcherError::EmptyTask);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= maxQueueSize_) {
            return std::unexpected(DispatcherError::QueueFull);
        }

        queue_.push_back(std::move(task));
        return {};
    }

    /**
     * @brief Wykonuje wszystkie zaplanowane zadania. Funkcja ta musi byc wywolywana TYLKO z glownego watku (Main Thread Tick).
     */
    void Process() {
        std::vector<Task> tasksToProcess;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return;
            }
            tasksToProcess = std::move(queue_);
            // std::move na vector pozostawia go w poprawnym (pustym) stanie.
            // Opcjonalnie mozemy wykonac queue_.reserve, ale zostawmy to puste, aby nie lokowac niepotrzebnie.
            queue_.clear();
        }

        // Wykonanie poza sekcja krytyczna, unikamy zakleszczen w razie gdy zadanie chce znow cos zglosic.
        for (const auto& task : tasksToProcess) {
            if (task) {
                task();
            }
        }
    }

    /**
     * @brief Czysci kolejke nie wykonujac zadan.
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    /**
     * @brief Zwraca aktualna liczbe zadan oczekujacych w kolejce.
     */
    size_t GetPendingCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Ustawia maksymalny limit zadan.
     */
    void SetMaxQueueSize(size_t maxSize) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxQueueSize_ = maxSize;
    }

private:
    QueuedEventDispatcher() : maxQueueSize_(DEFAULT_MAX_QUEUE_SIZE) {}
    ~QueuedEventDispatcher() = default;

    mutable std::mutex mutex_;
    std::vector<Task> queue_;
    size_t maxQueueSize_;
};

} // namespace Client::Core

template <>
struct std::formatter<Client::Core::DispatcherError> : std::formatter<std::string_view> {
    auto format(Client::Core::DispatcherError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};

