#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <expected>
#include <chrono>
#include <string_view>

#include "LogModern.h"
#include "Result.h"

/**
 * @file ThreadSafeQueue.h
 * @brief Nowoczesna, bezpieczna dla watkow kolejka oparta na standardzie C++23.
 */

namespace EterBase {

/**
 * @class ThreadSafeQueue
 * @brief Szablon bezpiecznej wielowatkowej kolejki.
 * 
 * Klasa implementuje podstawowe operacje wstawiania (Push) oraz pobierania elementow
 * z kolejki w sposob bezpieczny dla wielu watkow, przy uzyciu std::mutex oraz std::condition_variable.
 *
 * @tparam T Typ elementow przechowywanych w kolejce.
 */
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;
    
    // Blokujemy kopiowanie i przenoszenie
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

    /**
     * @brief Dodaje element na koniec kolejki.
     * @param value Wartosc elementu do dodania.
     */
    void Push(T value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(value));
        condition.notify_one();
    }

    /**
     * @brief Probuje pobrac element z kolejki bez oczekiwania.
     * @return Zwraca std::optional<T> z elementem jesli kolejka nie jest pusta, w przeciwnym razie std::nullopt.
     */
    [[nodiscard]] std::optional<T> TryPop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue.front());
        queue.pop();
        return value;
    }

    /**
     * @brief Oczekuje na pojawienie sie elementu w kolejce, a nastepnie go pobiera.
     * 
     * Blokuje biezacy watek do momentu pojawienia sie przynajmniej jednego elementu.
     * @return Pobrany element typu T.
     */
    [[nodiscard]] T WaitAndPop() {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return !queue.empty(); });
        T value = std::move(queue.front());
        queue.pop();
        return value;
    }

    /**
     * @brief Oczekuje na element przez okreslony czas.
     * 
     * Jesli w danym czasie element pojawi sie w kolejce, jest pobierany i zwracany jako sukces.
     * W przypadku uplyniecia limitu czasu, zwracany jest blad i logowane jest ostrzezenie.
     *
     * @param timeout_ms Czas oczekiwania w milisekundach.
     * @return std::expected<T, std::string_view> zawierajace wartosc lub blad "Timeout".
     */
    [[nodiscard]] std::expected<T, std::string_view> WaitAndPopTimeout(std::chrono::milliseconds timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!condition.wait_for(lock, timeout_ms, [this] { return !queue.empty(); })) {
            ModernLogger::Warn("ThreadSafeQueue WaitAndPopTimeout: Timeout of {} ms reached while waiting for an element.", timeout_ms.count());
            return std::unexpected("Timeout");
        }
        T value = std::move(queue.front());
        queue.pop();
        return value;
    }

    /**
     * @brief Sprawdza czy kolejka jest pusta.
     * @return true, jesli kolejka nie posiada elementow, false w przeciwnym wypadku.
     */
    [[nodiscard]] bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
    
    /**
     * @brief Zwraca liczbe elementow w kolejce.
     * @return Liczba elementow (size_t).
     */
    [[nodiscard]] size_t GetSize() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    /**
     * @brief Czysci zawartosc kolejki.
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex);
        std::queue<T> empty;
        std::swap(queue, empty);
    }

private:
    mutable std::mutex mutex;               ///< Mutex do zabezpieczenia dostepu do kolejki.
    std::condition_variable condition;      ///< Zmienna warunkowa do powiadamiania oczekujacych watkow.
    std::queue<T> queue;                    ///< Struktura danych przechowujaca elementy.
};

} // namespace EterBase
