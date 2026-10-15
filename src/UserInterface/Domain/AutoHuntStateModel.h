#pragma once

#include <optional>
#include <expected>
#include <format>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace Domain::Events {

#pragma pack(push, 1)

    /**
     * @brief Represents the event triggered when AutoHunt is started.
     */
    struct AutoHuntStarted : public UserInterface::Core::IEvent {
        float huntRadius; ///< The radius within which the character will hunt.
        explicit AutoHuntStarted(float radius) : huntRadius(radius) {}
    };

    /**
     * @brief Represents the event triggered when AutoHunt is stopped.
     */
    struct AutoHuntStopped : public UserInterface::Core::IEvent {
    };

    /**
     * @brief Represents the event triggered when the AutoHunt target changes.
     */
    struct AutoHuntTargetChanged : public UserInterface::Core::IEvent {
        uint32_t targetId; ///< The unique identifier of the new target entity.
        explicit AutoHuntTargetChanged(uint32_t id) : targetId(id) {}
    };

    /**
     * @brief Represents the event triggered when the AutoHunt radius changes.
     */
    struct AutoHuntRadiusChanged : public UserInterface::Core::IEvent {
        float newRadius; ///< The new hunt radius.
        explicit AutoHuntRadiusChanged(float radius) : newRadius(radius) {}
    };

    /**
     * @brief Represents the event triggered when the character returns to base during AutoHunt.
     */
    struct AutoHuntReturningToBase : public UserInterface::Core::IEvent {
        int32_t baseX; ///< The X coordinate of the base location.
        int32_t baseY; ///< The Y coordinate of the base location.
        AutoHuntReturningToBase(int32_t x, int32_t y) : baseX(x), baseY(y) {}
    };

#pragma pack(pop)

} // namespace Domain::Events

namespace Domain {

    /**
     * @brief Model representing the state of autonomous hunting (AutoHunt).
     * 
     * This class manages the state related to the AutoHunt feature, including
     * the active target, hunting radius, and the base location to return to.
     * It operates entirely independently of the GUI and publishes events via
     * the UserInterface::Core::EventBus upon state changes.
     */
    class AutoHuntStateModel {
    public:
        /**
         * @brief Represents a 2D position for the base location.
         */
        struct Position {
            int32_t x; ///< X coordinate.
            int32_t y; ///< Y coordinate.
        };

        /**
         * @brief Constructs a new AutoHuntStateModel with default values.
         */
        AutoHuntStateModel() = default;

        /**
         * @brief Starts the AutoHunt mode.
         * 
         * @return EterBase::VoidResult Success if successfully started, or an error if already active.
         */
        EterBase::VoidResult<> Start() {
            if (isActive) {
                EterBase::ModernLogger::Warn("Failed to start AutoHunt: already active.");
                return EterBase::MakeError("AutoHunt is already active");
            }

            isActive = true;
            EterBase::ModernLogger::Info("AutoHunt started with radius: {}", huntRadius);

            UserInterface::Core::EventBus::GetInstance().Publish(Events::AutoHuntStarted{huntRadius});

            return {};
        }

        /**
         * @brief Stops the AutoHunt mode.
         * 
         * @return EterBase::VoidResult Success if successfully stopped, or an error if not active.
         */
        EterBase::VoidResult<> Stop() {
            if (!isActive) {
                EterBase::ModernLogger::Warn("Failed to stop AutoHunt: not active.");
                return EterBase::MakeError("AutoHunt is not active");
            }

            isActive = false;
            ClearTarget();
            EterBase::ModernLogger::Info("AutoHunt stopped.");

            UserInterface::Core::EventBus::GetInstance().Publish(Events::AutoHuntStopped{});

            return {};
        }

        /**
         * @brief Sets a new target for AutoHunt.
         * 
         * @param newTarget The new entity ID to target.
         * @return EterBase::VoidResult Success if set, or an error if not active.
         */
        EterBase::VoidResult<> SetTarget(EterBase::EntityId newTarget) {
            if (!isActive) {
                return EterBase::MakeError("Cannot set target while AutoHunt is inactive");
            }

            target = newTarget;
            EterBase::ModernLogger::Debug("AutoHunt target set to ID: {}", newTarget.value());

            UserInterface::Core::EventBus::GetInstance().Publish(Events::AutoHuntTargetChanged{newTarget.value()});

            return {};
        }

        /**
         * @brief Clears the current AutoHunt target.
         * 
         * @return EterBase::VoidResult Always returns success.
         */
        EterBase::VoidResult<> ClearTarget() {
            target = std::nullopt;
            EterBase::ModernLogger::Debug("AutoHunt target cleared.");
            return {};
        }

        /**
         * @brief Sets the hunting radius.
         * 
         * @param radius The new radius. Must be greater than 0.
         * @return EterBase::VoidResult Success if updated, error if invalid radius.
         */
        EterBase::VoidResult<> SetHuntRadius(float radius) {
            if (radius <= 0.0f) {
                return EterBase::MakeError("Invalid hunt radius");
            }

            huntRadius = radius;
            EterBase::ModernLogger::Debug("AutoHunt radius changed to: {}", radius);

            UserInterface::Core::EventBus::GetInstance().Publish(Events::AutoHuntRadiusChanged{radius});

            return {};
        }

        /**
         * @brief Sets the base location.
         * 
         * @param pos The new base position.
         * @return EterBase::VoidResult Always returns success.
         */
        EterBase::VoidResult<> SetBaseLocation(Position pos) {
            baseLocation = pos;
            return {};
        }

        /**
         * @brief Triggers the return to base behavior.
         * 
         * @return EterBase::VoidResult Success if returning, error if base location is not set or not active.
         */
        EterBase::VoidResult<> ReturnToBase() {
            if (!isActive) {
                return EterBase::MakeError("Cannot return to base: AutoHunt is not active");
            }

            if (!baseLocation.has_value()) {
                return EterBase::MakeError("Cannot return to base: base location is not set");
            }

            ClearTarget();
            
            EterBase::ModernLogger::Info("AutoHunt returning to base location: ({}, {})", baseLocation->x, baseLocation->y);

            UserInterface::Core::EventBus::GetInstance().Publish(Events::AutoHuntReturningToBase{baseLocation->x, baseLocation->y});

            return {};
        }

        /**
         * @brief Checks if AutoHunt is currently active.
         * 
         * @return bool True if active, false otherwise.
         */
        [[nodiscard]] bool IsActive() const noexcept {
            return isActive;
        }

        /**
         * @brief Gets the current target, if any.
         * 
         * @return std::optional<EterBase::EntityId> The target ID or nullopt.
         */
        [[nodiscard]] std::optional<EterBase::EntityId> GetTarget() const noexcept {
            return target;
        }

        /**
         * @brief Gets the current hunt radius.
         * 
         * @return float The hunt radius.
         */
        [[nodiscard]] float GetHuntRadius() const noexcept {
            return huntRadius;
        }

    private:
        bool isActive{false}; ///< True if AutoHunt is currently running.
        float huntRadius{1000.0f}; ///< The radius within which to search for targets.
        std::optional<EterBase::EntityId> target{std::nullopt}; ///< The current active target entity.
        std::optional<Position> baseLocation{std::nullopt}; ///< The base location to return to.
    };

} // namespace Domain
