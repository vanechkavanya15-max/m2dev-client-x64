#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <string_view>
#include <format>
#include <expected>
#include "EventBus.h"
#include "Result.h"

namespace Client::Core {

/**
 * @brief Typy bledow dla EventSubscriber.
 */
enum class EventSubscriberError : uint8_t {
    None = 0,
    NullInstance,
    InvalidParameter
};

[[nodiscard]] inline constexpr std::string_view to_string(EventSubscriberError error) noexcept {
    switch (error) {
        case EventSubscriberError::None: return "EventSubscriberError::None";
        case EventSubscriberError::NullInstance: return "EventSubscriberError::NullInstance - Provided instance pointer is null";
        case EventSubscriberError::InvalidParameter: return "EventSubscriberError::InvalidParameter - Provided callback is invalid";
        default: return "EventSubscriberError::Unknown";
    }
}

/**
 * @brief Bezpieczny uchwyt subskrypcji zdarzen zapobiegajacy wyciekom pamieci.
 *        Wykorzystuje RAII do automatycznego odpinania przy niszczeniu obiektu.
 */
class EventSubscriber {
public:
    EventSubscriber() = default;

    // Brak mozliwosci kopiowania (unikalne wlascicielstwo cyklu zycia)
    EventSubscriber(const EventSubscriber&) = delete;
    EventSubscriber& operator=(const EventSubscriber&) = delete;

    // Przenoszenie jest dozwolone i bezpieczne
    EventSubscriber(EventSubscriber&& other) noexcept 
        : unsubscribers_(std::move(other.unsubscribers_)) {
    }

    EventSubscriber& operator=(EventSubscriber&& other) noexcept {
        if (this != &other) {
            UnsubscribeAll();
            unsubscribers_ = std::move(other.unsubscribers_);
        }
        return *this;
    }

    ~EventSubscriber() {
        UnsubscribeAll();
    }

    /**
     * @brief Subskrybuje zdarzenie dla wolnej funkcji lub lambdy.
     * 
     * @tparam EventType Typ zdarzenia
     * @param callback Funkcja wywolywana po wystapieniu zdarzenia
     * @return Result<void, EventSubscriberError>
     */
    template <typename EventType>
    Result<void, EventSubscriberError> Subscribe(std::function<void(const EventType&)> callback) {
        if (!callback) {
            return std::unexpected(EventSubscriberError::InvalidParameter);
        }

        auto& bus = EventBus::GetInstance();
        uint32_t subId = bus.template Subscribe<EventType>(std::move(callback));
        
        unsubscribers_.emplace_back([subId]() {
            EventBus::GetInstance().template Unsubscribe<EventType>(subId);
        });

        return {};
    }

    /**
     * @brief Subskrybuje zdarzenie dla metody klasy instancji.
     * 
     * @tparam EventType Typ zdarzenia
     * @tparam T Typ klasy
     * @param instance Wskaznik na instancje (nieposiadajacy, tylko do wywolania)
     * @param memberFunction Wskaznik na metode klasy
     * @return Result<void, EventSubscriberError>
     */
    template <typename EventType, typename T>
    Result<void, EventSubscriberError> Subscribe(T* instance, void (T::*memberFunction)(const EventType&)) {
        if (!instance) {
            return std::unexpected(EventSubscriberError::NullInstance);
        }
        if (!memberFunction) {
            return std::unexpected(EventSubscriberError::InvalidParameter);
        }

        auto callback = [instance, memberFunction](const EventType& event) {
            (instance->*memberFunction)(event);
        };

        return Subscribe<EventType>(std::move(callback));
    }

    /**
     * @brief Odpina wszystkie zarejestrowane subskrypcje manualnie (zwykle RAII robi to automatycznie).
     */
    void UnsubscribeAll() noexcept {
        for (const auto& unsubscriber : unsubscribers_) {
            if (unsubscriber) {
                unsubscriber();
            }
        }
        unsubscribers_.clear();
    }

private:
    std::vector<std::function<void()>> unsubscribers_;
};

} // namespace Client::Core

template <>
struct std::formatter<Client::Core::EventSubscriberError> : std::formatter<std::string_view> {
    auto format(Client::Core::EventSubscriberError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
