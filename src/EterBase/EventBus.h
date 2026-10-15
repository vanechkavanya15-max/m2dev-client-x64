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
#include "StrongTypes.h"

namespace EterBase {

/**
 * @brief Bazowy interfejs dla zdarzen polimorficznych w systemie EventBus (C++23).
 */
struct IEvent {
    virtual ~IEvent() = default;
};

/**
 * @brief Type-erased bazowy handler zdarzen.
 */
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void Execute(const void* eventPtr) = 0;
};

/**
 * @brief Generyczny handler dla konkretnego typu zdarzenia.
 */
template <typename EventType>
class EventHandler : public IEventHandler {
public:
    using Callback = std::function<void(const EventType&)>;

    explicit EventHandler(Callback callback) : callback_(std::move(callback)) {}

    void Execute(const void* eventPtr) override {
        callback_(*reinterpret_cast<const EventType*>(eventPtr));
    }

private:
    Callback callback_;
};

/**
 * @brief Uniwersalna, bezpieczna watkowo szyna zdarzen (Publish-Subscribe EventBus C++23) w rdzeniu EterBase.
 */
class EventBus {
public:
    static EventBus& GetInstance() {
        static EventBus instance;
        return instance;
    }

    static EventBus& Instance() {
        return GetInstance();
    }

    template <typename EventType>
    uint32_t Subscribe(std::function<void(const EventType&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t subId = ++nextSubscriptionId_;
        
        auto handler = std::make_shared<EventHandler<EventType>>(std::move(callback));
        subscribers_[typeid(EventType)].emplace_back(subId, handler);
        
        return subId;
    }

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

    template <typename EventType>
    void Publish(const EventType& event) {
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

        for (const auto& handler : handlersToExecute) {
            handler->Execute(&event);
        }
    }

private:
    EventBus() : nextSubscriptionId_(0) {}
    ~EventBus() = default;
    
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    std::mutex mutex_;
    std::atomic<uint32_t> nextSubscriptionId_;
    std::unordered_map<std::type_index, std::vector<std::pair<uint32_t, std::shared_ptr<IEventHandler>>>> subscribers_;
};

// Podstawowe zdarzenia systemowe
struct TargetBoardRefreshEvent : public IEvent {
    uint32_t targetId;
    explicit TargetBoardRefreshEvent(uint32_t targetId) : targetId(targetId) {}
};

struct NetworkPacketReceivedEvent : public IEvent {
    uint8_t header;
    std::span<const uint8_t> payload;
    NetworkPacketReceivedEvent(uint8_t header, std::span<const uint8_t> payload)
        : header(header), payload(payload) {}
};

struct MountStateChangedEvent : public IEvent {
    uint32_t charId;
    uint32_t mountVnum;
    uint8_t pos;
    MountStateChangedEvent(uint32_t charId = 0, uint32_t mountVnum = 0, uint8_t pos = 0)
        : charId(charId), mountVnum(mountVnum), pos(pos) {}
};

struct TextTailVisibilityChangedEvent : public IEvent {
    uint32_t entityId;
    bool isVisible;
    TextTailVisibilityChangedEvent(uint32_t id = 0, bool vis = false)
        : entityId(id), isVisible(vis) {}
};

struct SIMDCullingCompletedEvent : public IEvent {
    size_t totalCount;
    size_t visibleCount;
    SIMDCullingCompletedEvent(size_t total = 0, size_t visible = 0)
        : totalCount(total), visibleCount(visible) {}
};

struct ItemTooltipCachedEvent : public IEvent {
    EterBase::ItemVnum vnum;
    explicit ItemTooltipCachedEvent(EterBase::ItemVnum v = EterBase::ItemVnum{0}) : vnum(v) {}
};

struct AnimHitFrameEvent : public IEvent {
    EterBase::EntityId entityId;
    uint32_t motionKey;
    uint8_t hitIndex;
    AnimHitFrameEvent(EterBase::EntityId id = EterBase::EntityId{0}, uint32_t key = 0, uint8_t hit = 0)
        : entityId(id), motionKey(key), hitIndex(hit) {}
};

struct AnimFinishedEvent : public IEvent {
    EterBase::EntityId entityId;
    uint32_t motionKey;
    AnimFinishedEvent(EterBase::EntityId id = EterBase::EntityId{0}, uint32_t key = 0)
        : entityId(id), motionKey(key) {}
};

struct CustomTitleChangedEvent : public IEvent {
    std::string title;
    uint32_t color;
    CustomTitleChangedEvent(std::string_view t = "", uint32_t c = 0)
        : title(t), color(c) {}
};

struct ActorDeadEvent : public IEvent {
    uint32_t entityId{0};
    uint32_t vid{0};
    ActorDeadEvent() = default;
    explicit ActorDeadEvent(uint32_t id) : entityId(id), vid(id) {}
};

} // namespace EterBase

namespace UserInterface::Core {
    using IEvent = ::EterBase::IEvent;
    using IEventHandler = ::EterBase::IEventHandler;
    template <typename EventType>
    using EventHandler = ::EterBase::EventHandler<EventType>;
    using EventBus = ::EterBase::EventBus;

    using TargetBoardRefreshEvent = ::EterBase::TargetBoardRefreshEvent;
    using NetworkPacketReceivedEvent = ::EterBase::NetworkPacketReceivedEvent;
    using MountStateChangedEvent = ::EterBase::MountStateChangedEvent;
    using ActorDeadEvent = ::EterBase::ActorDeadEvent;
    using TextTailVisibilityChangedEvent = ::EterBase::TextTailVisibilityChangedEvent;
    using SIMDCullingCompletedEvent = ::EterBase::SIMDCullingCompletedEvent;
    using ItemTooltipCachedEvent = ::EterBase::ItemTooltipCachedEvent;
    using AnimHitFrameEvent = ::EterBase::AnimHitFrameEvent;
    using AnimFinishedEvent = ::EterBase::AnimFinishedEvent;
    using CustomTitleChangedEvent = ::EterBase::CustomTitleChangedEvent;
}

namespace Core {
    using IEvent = ::EterBase::IEvent;
    using EventBus = ::EterBase::EventBus;
    using TargetBoardRefreshEvent = ::EterBase::TargetBoardRefreshEvent;
    using NetworkPacketReceivedEvent = ::EterBase::NetworkPacketReceivedEvent;
    using MountStateChangedEvent = ::EterBase::MountStateChangedEvent;
    using ActorDeadEvent = ::EterBase::ActorDeadEvent;
    using TextTailVisibilityChangedEvent = ::EterBase::TextTailVisibilityChangedEvent;
    using SIMDCullingCompletedEvent = ::EterBase::SIMDCullingCompletedEvent;
    using ItemTooltipCachedEvent = ::EterBase::ItemTooltipCachedEvent;
    using AnimHitFrameEvent = ::EterBase::AnimHitFrameEvent;
    using AnimFinishedEvent = ::EterBase::AnimFinishedEvent;
    using CustomTitleChangedEvent = ::EterBase::CustomTitleChangedEvent;
}
