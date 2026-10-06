#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <span>
#include <string_view>
#include <mutex>
#include <atomic>

namespace UserInterface::Core {

/**
 * @brief Base interface for all events in the EventBus system.
 */
struct IEvent {
    virtual ~IEvent() = default;
};

/**
 * @brief Event triggered to request a refresh of the target board UI.
 * 
 * Replaces direct Python UI calls (e.g., PyCallClassMemberFunc) to achieve decoupling.
 */
struct TargetBoardRefreshEvent : public IEvent {
    uint32_t targetId;

    /**
     * @brief Constructs the target board refresh event.
     * @param targetId The unique identifier of the target to refresh.
     */
    explicit TargetBoardRefreshEvent(uint32_t targetId) : targetId(targetId) {}
};

/**
 * @brief Network packet received event.
 * Note: Packing (#pragma pack) is deliberately NOT used here because this 
 * struct inherits from IEvent (virtual vtable) and contains std::span. 
 * Packing polymorphic/complex types leads to undefined behavior.
 * We store the unaligned binary data inside the span.
 */
struct NetworkPacketReceivedEvent : public IEvent {
    uint8_t header;
    std::span<const uint8_t> payload;

    /**
     * @brief Constructs the network packet event.
     * @param header The packet opcode.
     * @param payload The binary data of the packet payload.
     */
    NetworkPacketReceivedEvent(uint8_t header, std::span<const uint8_t> payload)
        : header(header), payload(payload) {}
};

/**
 * @brief Mount state changed event.
 */
struct MountStateChangedEvent : public IEvent {
    uint32_t charId;
    uint32_t mountVnum;
    uint8_t pos;

    MountStateChangedEvent(uint32_t charId = 0, uint32_t mountVnum = 0, uint8_t pos = 0)
        : charId(charId), mountVnum(mountVnum), pos(pos) {}
};

/**
 * @brief Type-erased base handler for events.
 */
class IEventHandler {
public:
    virtual ~IEventHandler() = default;

    /**
     * @brief Executes the handler with the given event.
     * @param eventPtr The event pointer to process.
     */
    virtual void Execute(const void* eventPtr) = 0;
};

/**
 * @brief A type-specific handler for a given event type.
 * @tparam EventType The specific type of the event.
 */
template <typename EventType>
class EventHandler : public IEventHandler {
public:
    using Callback = std::function<void(const EventType&)>;

    /**
     * @brief Constructs the handler with a callback.
     * @param callback The function to execute when the event occurs.
     */
    explicit EventHandler(Callback callback) : callback_(std::move(callback)) {}

    /**
     * @brief Executes the callback, casting the raw pointer to the specific type.
     * @param eventPtr The raw event pointer to process.
     */
    void Execute(const void* eventPtr) override {
        callback_(*reinterpret_cast<const EventType*>(eventPtr));
    }

private:
    Callback callback_;
};

/**
 * @brief A universal event bus for publish-subscribe messaging.
 * 
 * Implements a thread-safe EventBus to decouple different components of the system.
 * Subsystems can subscribe to specific EventTypes and get notified when they occur.
 */
class EventBus {
public:
    /**
     * @brief Gets the singleton instance of the EventBus.
     * @return Reference to the EventBus instance.
     */
    static EventBus& GetInstance() {
        static EventBus instance;
        return instance;
    }

    static EventBus& Instance() {
        return GetInstance();
    }

    /**
     * @brief Subscribes a callback to a specific event type.
     * @tparam EventType The type of event to subscribe to.
     * @param callback The callback function to be executed when the event is published.
     * @return A unique subscription ID for unsubscribing.
     */
    template <typename EventType>
    uint32_t Subscribe(std::function<void(const EventType&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t subId = ++nextSubscriptionId_;
        
        auto handler = std::make_shared<EventHandler<EventType>>(std::move(callback));
        subscribers_[typeid(EventType)].emplace_back(subId, handler);
        
        return subId;
    }

    /**
     * @brief Unsubscribes a specific handler by its subscription ID.
     * @tparam EventType The type of the event to unsubscribe from.
     * @param subscriptionId The ID returned during subscription.
     */
    template <typename EventType>
    void Unsubscribe(uint32_t subscriptionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(typeid(EventType));
        if (it != subscribers_.end()) {
            auto& list = it->second;
            std::erase_if(list, [subscriptionId](const auto& pair) {
                return pair.first == subscriptionId;
            });
        }
    }

    /**
     * @brief Publishes an event to all subscribers of its type.
     * @tparam EventType The type of the event.
     * @param event The event instance to publish.
     */
    template <typename EventType>
    void Publish(const EventType& event) {
        // Collect handlers securely without executing them while holding the lock
        // to prevent potential deadlocks if a callback calls EventBus methods.
        std::vector<std::shared_ptr<IEventHandler>> handlersToExecute;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(typeid(EventType));
            if (it != subscribers_.end()) {
                for (const auto& [id, handler] : it->second) {
                    handlersToExecute.push_back(handler);
                }
            }
        }

        // Execute collected handlers
        for (const auto& handler : handlersToExecute) {
            handler->Execute(&event);
        }
    }

private:
    EventBus() : nextSubscriptionId_(0) {}
    ~EventBus() = default;
    
    // Non-copyable and non-movable
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    std::mutex mutex_;
    std::atomic<uint32_t> nextSubscriptionId_;
    std::unordered_map<std::type_index, std::vector<std::pair<uint32_t, std::shared_ptr<IEventHandler>>>> subscribers_;
};

} // namespace UserInterface::Core

namespace Core {
    using EventBus = ::UserInterface::Core::EventBus;
}
