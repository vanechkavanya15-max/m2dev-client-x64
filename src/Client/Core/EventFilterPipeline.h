#pragma once

#include <chrono>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <memory>
#include <string_view>
#include <format>
#include <expected>
#include "Result.h"

namespace Client::Core {

enum class EventFilterError : uint8_t {
    Throttled,
    Duplicate
};

[[nodiscard]] constexpr std::string_view to_string(EventFilterError error) noexcept {
    switch (error) {
        case EventFilterError::Throttled: return "Throttled";
        case EventFilterError::Duplicate: return "Duplicate";
    }
    return "Unknown";
}

/**
 * @brief Ogranicznik czestotliwosci rozsylania zdarzen o wysokiej intensywnosci.
 * Zapewnia deduplikacje oraz throttling na poziomie rury z filtrami.
 */
template <typename Clock = std::chrono::steady_clock>
class EventFilterPipeline {
public:
    using TimePoint = typename Clock::time_point;
    using Duration = typename Clock::duration;

    EventFilterPipeline() = default;

    EventFilterPipeline(const EventFilterPipeline&) = delete;
    EventFilterPipeline& operator=(const EventFilterPipeline&) = delete;
    EventFilterPipeline(EventFilterPipeline&&) noexcept = default;
    EventFilterPipeline& operator=(EventFilterPipeline&&) noexcept = default;

    template <typename EventType>
    void SetThrottleInterval(Duration interval) {
        throttleConfig_[std::type_index(typeid(EventType))] = interval;
    }

    template <typename EventType>
    void EnableDeduplication() {
        dedupConfig_[std::type_index(typeid(EventType))] = true;
    }

    template <typename EventType>
    void DisableDeduplication() {
        dedupConfig_[std::type_index(typeid(EventType))] = false;
    }

    template <typename EventType>
    [[nodiscard]] Result<void, EventFilterError> Process(const EventType& event, TimePoint now = Clock::now()) {
        auto typeIdx = std::type_index(typeid(EventType));

        // Sprawdzenie Throttling
        if (auto it = throttleConfig_.find(typeIdx); it != throttleConfig_.end()) {
            auto lastTimeIt = lastEventTimes_.find(typeIdx);
            if (lastTimeIt != lastEventTimes_.end()) {
                if (now - lastTimeIt->second < it->second) {
                    return std::unexpected(EventFilterError::Throttled);
                }
            }
        }

        // Sprawdzenie Deduplikacji
        if (auto it = dedupConfig_.find(typeIdx); it != dedupConfig_.end() && it->second) {
            auto lastEventIt = lastEvents_.find(typeIdx);
            if (lastEventIt != lastEvents_.end()) {
                if (lastEventIt->second->IsEqual(&event)) {
                    return std::unexpected(EventFilterError::Duplicate);
                }
            }
        }

        // Aktualizacja stanu jesli zdarzenie przeszlo przez filtry
        if (throttleConfig_.contains(typeIdx)) {
            lastEventTimes_[typeIdx] = now;
        }

        if (auto it = dedupConfig_.find(typeIdx); it != dedupConfig_.end() && it->second) {
            lastEvents_[typeIdx] = std::make_unique<EventHolder<EventType>>(event);
        }

        return {};
    }

    void Clear() {
        lastEventTimes_.clear();
        lastEvents_.clear();
    }

private:
    struct IEventHolder {
        virtual ~IEventHolder() = default;
        virtual bool IsEqual(const void* other) const = 0;
    };

    template <typename EventType>
    struct EventHolder : IEventHolder {
        EventType event;
        explicit EventHolder(const EventType& e) : event(e) {}
        
        bool IsEqual(const void* other) const override {
            if constexpr (requires { event == *static_cast<const EventType*>(other); }) {
                return event == *static_cast<const EventType*>(other);
            } else {
                return false;
            }
        }
    };

    std::unordered_map<std::type_index, Duration> throttleConfig_;
    std::unordered_map<std::type_index, TimePoint> lastEventTimes_;
    
    std::unordered_map<std::type_index, bool> dedupConfig_;
    std::unordered_map<std::type_index, std::unique_ptr<IEventHolder>> lastEvents_;
};

} // namespace Client::Core

template <>
struct std::formatter<Client::Core::EventFilterError> : std::formatter<std::string_view> {
    auto format(Client::Core::EventFilterError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
