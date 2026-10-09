#pragma once

#include <cstdint>
#include <expected>
#include <array>
#include <mutex>
#include <string_view>
#include <utility>


namespace Client::Core {

enum class EventError : uint8_t {
    CapacityExceeded,
    SubscriberNotFound,
    InvalidHandler
};

[[nodiscard]] constexpr std::string_view to_string(EventError error) noexcept {
    switch (error) {
        case EventError::CapacityExceeded: return "CapacityExceeded";
        case EventError::SubscriberNotFound: return "SubscriberNotFound";
        case EventError::InvalidHandler: return "InvalidHandler";
    }
    return "UnknownEventError";
}

template <typename EventType>
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void OnEvent(const EventType& event) noexcept = 0;
};

// Adapter dla wyrazen lambda (musi byc przetrzymywany po stronie subskrybenta jako lvalue)
template <typename EventType, typename Lambda>
class LambdaEventHandler final : public IEventHandler<EventType> {
public:
    explicit LambdaEventHandler(Lambda lambda) noexcept : lambda_(std::move(lambda)) {}
    
    void OnEvent(const EventType& event) noexcept override {
        lambda_(event);
    }
private:
    Lambda lambda_;
};

// Adapter dla metod klas (musi byc przetrzymywany po stronie subskrybenta jako lvalue)
template <typename EventType, class T>
class MemberEventHandler final : public IEventHandler<EventType> {
public:
    using MemberFunc = void (T::*)(const EventType&) noexcept;

    MemberEventHandler(T* instance, MemberFunc func) noexcept
        : instance_(instance), func_(func) {}

    void OnEvent(const EventType& event) noexcept override {
        if (instance_ && func_) {
            (instance_->*func_)(event);
        }
    }
private:
    T* instance_;
    MemberFunc func_;
};

class IEventChannel {
public:
    virtual ~IEventChannel() = default;
    virtual void Clear() noexcept = 0;
};

class CoreEventBus {
public:
    static constexpr size_t MaxChannels = 256;
    
    static CoreEventBus& Instance() noexcept;

    CoreEventBus(const CoreEventBus&) = delete;
    CoreEventBus& operator=(const CoreEventBus&) = delete;

    template <typename EventType>
    std::expected<uint32_t, EventError> Subscribe(IEventHandler<EventType>* handler) noexcept;

    template <typename EventType>
    std::expected<void, EventError> Unsubscribe(uint32_t id) noexcept;

    template <typename EventType>
    void Publish(const EventType& event) noexcept;

    void ClearAll() noexcept;
    void RegisterChannel(IEventChannel* channel) noexcept;

private:
    CoreEventBus() = default;
    ~CoreEventBus() = default;

    std::array<IEventChannel*, MaxChannels> channels_{};
    size_t channelCount_{0};
    std::mutex busMutex_;
};

template <typename EventType, size_t MaxSubscribers = 128>
class EventChannel final : public IEventChannel {
public:
    struct Subscriber {
        uint32_t id{0};
        IEventHandler<EventType>* handler{nullptr};
    };

    static EventChannel& Get() noexcept {
        static EventChannel instance;
        return instance;
    }

    EventChannel() noexcept {
        CoreEventBus::Instance().RegisterChannel(this);
    }

    std::expected<uint32_t, EventError> Subscribe(IEventHandler<EventType>* handler) noexcept {
        if (!handler) {
            return std::unexpected(EventError::InvalidHandler);
        }
        
        std::lock_guard lock(mutex_);
        if (count_ >= MaxSubscribers) {
            return std::unexpected(EventError::CapacityExceeded);
        }
        
        uint32_t newId = ++nextId_;
        subscribers_[count_] = Subscriber{newId, handler};
        ++count_;
        
        return newId;
    }

    std::expected<void, EventError> Unsubscribe(uint32_t id) noexcept {
        std::lock_guard lock(mutex_);
        
        for (size_t i = 0; i < count_; ++i) {
            if (subscribers_[i].id == id) {
                if (i != count_ - 1) {
                    subscribers_[i] = subscribers_[count_ - 1];
                }
                subscribers_[count_ - 1] = Subscriber{};
                --count_;
                return {};
            }
        }
        
        return std::unexpected(EventError::SubscriberNotFound);
    }

    void Publish(const EventType& event) noexcept {
        std::array<Subscriber, MaxSubscribers> localSubscribers;
        size_t localCount = 0;
        
        {
            std::lock_guard lock(mutex_);
            localCount = count_;
            for (size_t i = 0; i < localCount; ++i) {
                localSubscribers[i] = subscribers_[i];
            }
        }
        
        for (size_t i = 0; i < localCount; ++i) {
            if (localSubscribers[i].handler) {
                localSubscribers[i].handler->OnEvent(event);
            }
        }
    }

    void Clear() noexcept override {
        std::lock_guard lock(mutex_);
        count_ = 0;
    }

private:
    std::array<Subscriber, MaxSubscribers> subscribers_{};
    size_t count_{0};
    uint32_t nextId_{0};
    std::mutex mutex_;
};

template <typename EventType>
std::expected<uint32_t, EventError> CoreEventBus::Subscribe(IEventHandler<EventType>* handler) noexcept {
    return EventChannel<EventType>::Get().Subscribe(handler);
}

template <typename EventType>
std::expected<void, EventError> CoreEventBus::Unsubscribe(uint32_t id) noexcept {
    return EventChannel<EventType>::Get().Unsubscribe(id);
}

template <typename EventType>
void CoreEventBus::Publish(const EventType& event) noexcept {
    EventChannel<EventType>::Get().Publish(event);
}

} // namespace Client::Core
