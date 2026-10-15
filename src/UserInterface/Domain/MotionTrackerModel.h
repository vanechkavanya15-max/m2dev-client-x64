#pragma once

#include <chrono>
#include <optional>
#include <cstdint>
#include <string_view>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents the type of motion currently being tracked.
 */
enum class MotionType : uint8_t {
    None = 0,
    Idle,
    Run,
    Attack,
    Skill,
    Knockback,
    Dead
};

/**
 * @brief Event emitted when an actor's motion state changes.
 */
struct MotionStateChangedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    MotionType newMotionType;
    uint32_t motionKey;

    /**
     * @brief Constructs the motion state changed event.
     * @param id The ID of the entity whose motion changed.
     * @param type The new motion type.
     * @param key The specific motion key (vnum/hash).
     */
    MotionStateChangedEvent(EterBase::EntityId id, MotionType type, uint32_t key)
        : entityId(id), newMotionType(type), motionKey(key) {}
};

/**
 * @brief Event emitted when an animation explicitly finishes.
 */
struct MotionFinishedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    uint32_t motionKey;

    /**
     * @brief Constructs the motion finished event.
     * @param id The ID of the entity whose motion finished.
     * @param key The specific motion key that finished.
     */
    MotionFinishedEvent(EterBase::EntityId id, uint32_t key)
        : entityId(id), motionKey(key) {}
};

/**
 * @brief Tracks the motion state, running/attacking animations, and animation end times for an entity.
 * 
 * Fully decoupled from the GUI, utilizing C++23 features like std::optional and std::expected.
 * Updates state purely in memory and publishes events to the EventBus.
 */
class MotionTrackerModel {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;

    /**
     * @brief Constructs a new MotionTrackerModel for a specific entity.
     * @param id The unique entity identifier.
     */
    explicit MotionTrackerModel(EterBase::EntityId id) : entityId_(id) {}

    /**
     * @brief Destroys the MotionTrackerModel.
     */
    ~MotionTrackerModel() = default;

    /**
     * @brief Starts a new motion/animation.
     * 
     * @param type The category of the motion.
     * @param motionKey The specific identifier for the animation.
     * @param duration The exact length of the animation.
     * @param startTime The time the animation began.
     * @return EterBase::VoidResult<> A success result, or an error if invalid parameters were provided.
     */
    EterBase::VoidResult<> StartMotion(MotionType type, uint32_t motionKey, Duration duration, TimePoint startTime) {
        if (type == MotionType::None) {
            EterBase::ModernLogger::Warn("Attempted to start a None motion type for entity {}", entityId_.value());
            return EterBase::MakeError(std::string_view("Invalid motion type"));
        }

        currentMotionType_ = type;
        currentMotionKey_ = motionKey;
        motionEndTime_ = startTime + duration;

        Core::EventBus::GetInstance().Publish(MotionStateChangedEvent(entityId_, currentMotionType_, currentMotionKey_));
        
        EterBase::ModernLogger::Debug("Entity {} started motion {} (Type: {})", entityId_.value(), motionKey, static_cast<uint32_t>(type));
        return {};
    }

    /**
     * @brief Updates the tracker logic, checking if the current motion has finished.
     * 
     * @param currentTime The current simulation time.
     * @return std::optional<uint32_t> The finished motion key if it just ended, or std::nullopt if still playing/idle.
     */
    std::optional<uint32_t> Update(TimePoint currentTime) {
        return motionEndTime_.and_then([this, currentTime](TimePoint endTime) -> std::optional<uint32_t> {
            if (currentTime >= endTime) {
                uint32_t finishedKey = currentMotionKey_;
                
                Core::EventBus::GetInstance().Publish(MotionFinishedEvent(entityId_, finishedKey));
                
                // Reset to Idle
                currentMotionType_ = MotionType::Idle;
                currentMotionKey_ = 0;
                motionEndTime_.reset();
                
                Core::EventBus::GetInstance().Publish(MotionStateChangedEvent(entityId_, currentMotionType_, currentMotionKey_));
                
                return finishedKey;
            }
            return std::nullopt;
        });
    }

    /**
     * @brief Checks if a specific type of motion is currently active.
     * 
     * @param type The motion type to check for.
     * @return true if the entity is currently performing this motion type.
     */
    [[nodiscard]] bool IsMotionTypeActive(MotionType type) const noexcept {
        return currentMotionType_ == type;
    }

    /**
     * @brief Checks if the entity is currently attacking.
     * 
     * @return true if attacking.
     */
    [[nodiscard]] bool IsAttacking() const noexcept {
        return currentMotionType_ == MotionType::Attack || currentMotionType_ == MotionType::Skill;
    }

    /**
     * @brief Checks if the entity is currently moving/running.
     * 
     * @return true if running.
     */
    [[nodiscard]] bool IsRunning() const noexcept {
        return currentMotionType_ == MotionType::Run;
    }

    /**
     * @brief Interrupts the current motion prematurely.
     */
    void InterruptMotion() {
        if (currentMotionType_ != MotionType::Idle && currentMotionType_ != MotionType::None) {
            EterBase::ModernLogger::Debug("Entity {} motion interrupted.", entityId_.value());
            
            currentMotionType_ = MotionType::Idle;
            currentMotionKey_ = 0;
            motionEndTime_.reset();
            
            Core::EventBus::GetInstance().Publish(MotionStateChangedEvent(entityId_, currentMotionType_, currentMotionKey_));
        }
    }

    /**
     * @brief Gets the current motion type.
     * 
     * @return The current MotionType.
     */
    [[nodiscard]] MotionType GetCurrentMotionType() const noexcept {
        return currentMotionType_;
    }

    /**
     * @brief Gets the current motion key.
     * 
     * @return The current motion key.
     */
    [[nodiscard]] uint32_t GetCurrentMotionKey() const noexcept {
        return currentMotionKey_;
    }

private:
    EterBase::EntityId entityId_;
    MotionType currentMotionType_{MotionType::None};
    uint32_t currentMotionKey_{0};
    std::optional<TimePoint> motionEndTime_;
};

} // namespace UserInterface::Domain
