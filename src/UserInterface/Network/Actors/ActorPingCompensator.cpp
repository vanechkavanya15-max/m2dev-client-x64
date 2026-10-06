#include "../../StdAfx.h"
#include "IMovementPredictionService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/ModernLogger.h"
#include "UserInterface/Core/EventBus.h"
#include <unordered_map>
#include <cmath>

namespace UserInterface::Network {

    /**
     * @brief Actor movement prediction service with ping (RTT) compensation.
     * Implementacja Zero-Conflict. Oczekuje polowy czasu RTT na korekte.
     */
    class ActorPingCompensator : public IMovementPredictionService {
    public:
        ActorPingCompensator() = default;
        ~ActorPingCompensator() override = default;

        /**
         * @brief Updates predicted position for an entity over time.
         * @param id The entity ID.
         * @param deltaTime Elapsed time in seconds.
         */
        void UpdatePrediction(EterBase::EntityId id, float deltaTime) override {
            auto it = m_entities.find(id);
            if (it == m_entities.end()) {
                return;
            }

            auto& state = it->second;
            if (state.speed <= 0.0f) {
                return;
            }

            float dx = state.targetX - state.current.x;
            float dy = state.targetY - state.current.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 0.01f) {
                float moveDist = state.speed * deltaTime;
                if (moveDist > dist) {
                    state.current.x = state.targetX;
                    state.current.y = state.targetY;
                    state.speed = 0.0f;
                } else {
                    state.current.x += (dx / dist) * moveDist;
                    state.current.y += (dy / dist) * moveDist;
                }
            }
        }

        /**
         * @brief Handles server position packets and applies half RTT ping compensation.
         * @param id The entity ID.
         * @param destX Target X coordinate from server.
         * @param destY Target Y coordinate from server.
         * @param speed Current movement speed.
         * @param serverTime Timestamp from the server.
         */
        void OnServerMovePacket(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) override {
            auto& state = m_entities[id];
            
            // Half RTT compensation value (50ms)
            const float halfRttCompensation = 0.05f; 
            
            state.targetX = destX;
            state.targetY = destY;
            state.speed = speed;
            
            float dx = destX - state.current.x;
            float dy = destY - state.current.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist > 0.0f) {
                float compDist = speed * halfRttCompensation;
                if (compDist < dist) {
                    state.current.x += (dx / dist) * compDist;
                    state.current.y += (dy / dist) * compDist;
                } else {
                    state.current.x = destX;
                    state.current.y = destY;
                }
            }
            
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "ActorPingCompensator: Move update for Entity {}, target({}, {}) speed {}", id.value(), destX, destY, speed);
        }

        /**
         * @brief Retrieves the latest predicted position for an entity.
         * @param id The entity ID.
         * @return The predicted position.
         */
        PredictedPosition GetPredictedPosition(EterBase::EntityId id) const override {
            auto it = m_entities.find(id);
            if (it != m_entities.end()) {
                return it->second.current;
            }
            return PredictedPosition{};
        }

        /**
         * @brief Resets/forgets an entity's position data.
         * @param id The entity ID.
         */
        void ResetEntity(EterBase::EntityId id) override {
            m_entities.erase(id);
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "ActorPingCompensator: Reset Entity {}", id.value());
        }

        /**
         * @brief Clears all entities from the predictor.
         */
        void Clear() override {
            m_entities.clear();
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "ActorPingCompensator: Cleared all entities");
        }

    private:
        struct EntityState {
            PredictedPosition current;
            float targetX{0.0f};
            float targetY{0.0f};
            float speed{0.0f};
        };

        std::unordered_map<EterBase::EntityId, EntityState> m_entities;
    };

} // namespace UserInterface::Network
