#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <optional>
#include <mutex>
#include <expected>
#include "EventBus.h"

namespace Client::Core {

/**
 * @brief Enumy bledow operacji MockEventListener.
 */
enum class MockError : uint8_t {
    None = 0,
    AlreadySubscribed,
    NotSubscribed
};

/**
 * @brief Rejestrator wywolan ulatwiajacy asercje zgloszonych zdarzen w testach.
 * Pozwala podsluchiwac, pobierac i sprawdzac wyslane przez EventBus eventy.
 */
template <typename EventType>
class MockEventListener {
public:
    MockEventListener() = default;

    ~MockEventListener() {
        (void)Unsubscribe();
    }

    // Blokujemy kopiowanie i przenoszenie
    MockEventListener(const MockEventListener&) = delete;
    MockEventListener& operator=(const MockEventListener&) = delete;

    /**
     * @brief Zapisuje nasluchiwacz do EventBus. Zwraca blad jesli juz jest zapisany.
     */
    std::expected<void, MockError> Subscribe(EventBus& bus = EventBus::Instance()) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (subscriptionId_.has_value()) {
            return std::unexpected(MockError::AlreadySubscribed);
        }

        busPtr_ = &bus;
        uint32_t id = busPtr_->Subscribe<EventType>([this](const EventType& event) {
            this->OnEvent(event);
        });
        
        subscriptionId_ = id;
        return {};
    }

    /**
     * @brief Wypisuje nasluchiwacz z EventBus.
     */
    std::expected<void, MockError> Unsubscribe() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!subscriptionId_.has_value() || !busPtr_) {
            return std::unexpected(MockError::NotSubscribed);
        }

        busPtr_->Unsubscribe<EventType>(subscriptionId_.value());
        subscriptionId_.reset();
        busPtr_ = nullptr;
        return {};
    }

    /**
     * @brief Udostepnia zdarzenia bez ich kopiowania (zwraca widok pamieci).
     */
    std::span<const EventType> InspectEvents() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::span<const EventType>(events_);
    }

    /**
     * @brief Zwraca ostatnie zdarzenie jezeli istnieje.
     */
    std::optional<EventType> GetLastEvent() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.empty()) {
            return std::nullopt;
        }
        return events_.back();
    }

    /**
     * @brief Cysci dotychczas zebrane zdarzenia.
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
    }

    /**
     * @brief Zwraca liczbe zebranych zdarzen.
     */
    size_t GetEventCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

private:
    void OnEvent(const EventType& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

    mutable std::mutex mutex_;
    std::vector<EventType> events_;
    std::optional<uint32_t> subscriptionId_;
    EventBus* busPtr_{nullptr};
};

} // namespace Client::Core
