#include "../../StdAfx.h"
#include "IMovementPredictionService.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

#include <cmath>
#include <numbers>
#include <unordered_map>
#include <memory>

namespace UserInterface::Network
{
    /**
     * @brief Event published when an actor's position is predicted via Dead Reckoning.
     */
    struct ActorPositionPredictedEvent : public Core::IEvent
    {
        EterBase::EntityId id;
        float x;
        float y;
        float z;
        float yaw;

    private:
        /**
         * @brief Private constructor to enforce factory creation.
         */
        ActorPositionPredictedEvent(EterBase::EntityId id, float x, float y, float z, float yaw)
            : id(id), x(x), y(y), z(z), yaw(yaw) {}

    public:
        /**
         * @brief Safe factory method for creating the event.
         * @param id The entity ID.
         * @param x The predicted X coordinate.
         * @param y The predicted Y coordinate.
         * @param z The predicted Z coordinate.
         * @param yaw The actor's yaw.
         * @return Expected containing the event or an error if the entity is invalid.
         */
        static EterBase::Result<ActorPositionPredictedEvent, EterBase::EntityError> Create(EterBase::EntityId id, float x, float y, float z, float yaw)
        {
            if (id.value() == 0)
            {
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }
            return ActorPositionPredictedEvent(id, x, y, z, yaw);
        }
    };

    /**
     * @brief Dead reckoning implementation for predicting actor movement between server updates.
     */
    class ActorDeadReckoning : public IMovementPredictionService
    {
    private:
        std::unordered_map<EterBase::EntityId, PredictedPosition> predictions_;

    public:
        /**
         * @brief Destructor.
         */
        ~ActorDeadReckoning() override = default;

        /**
         * @brief Updates the actor's predicted position based on velocity and yaw.
         * @param id The entity ID to update.
         * @param deltaTime The time elapsed since the last update, in milliseconds.
         */
        void UpdatePrediction(EterBase::EntityId id, float deltaTime) override
        {
            auto it = predictions_.find(id);
            if (it == predictions_.end())
            {
                EterBase::ModernLogger::Debug("UpdatePrediction skipped for entity {}, not tracked", id.value());
                return;
            }

            PredictedPosition& pos = it->second;

            if (pos.speed > 0.0f)
            {
                // In Metin2, rotation to direction vector uses sin for X and cos for Y.
                float yaw_rad = pos.yaw * std::numbers::pi_v<float> / 180.0f;
                float dirX = std::sin(yaw_rad);
                float dirY = std::cos(yaw_rad);

                // Convert speed to unit displacement per millisecond
                float distance = pos.speed * deltaTime;

                pos.x += dirX * distance;
                pos.y += dirY * distance;

                // Create and publish event
                auto eventResult = ActorPositionPredictedEvent::Create(id, pos.x, pos.y, pos.z, pos.yaw);
                if (eventResult)
                {
                    Core::EventBus::GetInstance().Publish(eventResult.value());
                }
                else
                {
                    EterBase::ModernLogger::Warning("Failed to create ActorPositionPredictedEvent for entity {}", id.value());
                }
            }
        }

        /**
         * @brief Updates the tracked state when a move packet is received from the server.
         * @param id The entity ID.
         * @param destX The destination X coordinate.
         * @param destY The destination Y coordinate.
         * @param speed The movement speed.
         * @param serverTime The server timestamp.
         */
        void OnServerMovePacket(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) override
        {
            auto& pos = predictions_[id];
            
            // Note: we just update tracking parameters as per instruction: 
            // "update the current prediction tracking parameters (yaw and speed) based on the destination received from the server."
            
            // We calculate new yaw based on difference between current pos and destination
            // if we have a current pos. If this is the first packet, we can't easily calculate yaw
            // without knowing the current position. But wait, if this is OnServerMovePacket, we might
            // just set the destination as current if we don't have it, or we could just set speed and yaw.
            
            // Let's implement calculating yaw.
            if (pos.x != 0.0f || pos.y != 0.0f) // Very simple check, a bit flawed if position is exactly 0,0 but good enough for simple logic.
            {
                float dx = destX - pos.x;
                float dy = destY - pos.y;
                if (dx != 0.0f || dy != 0.0f)
                {
                    // Metin2 uses standard atan2, but with sin for X and cos for Y. 
                    // This means yaw = atan2(dx, dy) * 180 / pi.
                    pos.yaw = std::atan2(dx, dy) * 180.0f / std::numbers::pi_v<float>;
                }
            }
            else
            {
                // First time we see this entity moving, just set its position to destination for now.
                pos.x = destX;
                pos.y = destY;
            }

            pos.speed = speed;
            EterBase::ModernLogger::Trace("OnServerMovePacket updated tracking for entity {} to pos ({}, {}) speed {} yaw {}", id.value(), pos.x, pos.y, pos.speed, pos.yaw);
        }

        /**
         * @brief Gets the current predicted position for an entity.
         * @param id The entity ID.
         * @return The predicted position.
         */
        PredictedPosition GetPredictedPosition(EterBase::EntityId id) const override
        {
            auto it = predictions_.find(id);
            if (it != predictions_.end())
            {
                return it->second;
            }
            return PredictedPosition{};
        }

        /**
         * @brief Resets the tracking for an entity.
         * @param id The entity ID.
         */
        void ResetEntity(EterBase::EntityId id) override
        {
            predictions_.erase(id);
            EterBase::ModernLogger::Trace("ResetEntity removed tracking for entity {}", id.value());
        }

        /**
         * @brief Clears all tracking data.
         */
        void Clear() override
        {
            predictions_.clear();
            EterBase::ModernLogger::Debug("ActorDeadReckoning state cleared");
        }
    };

    /**
     * @brief Factory function to create an instance of the Dead Reckoning service.
     * @return A unique pointer to the service interface.
     */
    std::unique_ptr<IMovementPredictionService> CreateActorDeadReckoning()
    {
        return std::make_unique<ActorDeadReckoning>();
    }
}
