#include "../../StdAfx.h"
#include "IMovementPredictionService.h"
#include "../../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <mutex>

namespace UserInterface::Core {
    struct ActorMovementPredictedEvent : public IEvent {
        EterBase::EntityId entityId;
        float x;
        float y;
        float speed;

        ActorMovementPredictedEvent(EterBase::EntityId id, float newX, float newY, float currentSpeed)
            : entityId(id), x(newX), y(newY), speed(currentSpeed) {}
    };
}

namespace UserInterface::Network {

    class ActorJitterBuffer : public IMovementPredictionService {
    private:
        struct MovementState {
            PredictedPosition currentPos;
            PredictedPosition targetPos;
            uint32_t lastServerTime{0};
            bool isMoving{false};
        };

        std::unordered_map<EterBase::EntityId, MovementState> m_states;
        mutable std::mutex m_mutex;

    public:
        EterBase::Result<void, EterBase::EntityError> UpdatePredictionModern(EterBase::EntityId id, float deltaTime) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_states.find(id);
            if (it == m_states.end()) return EterBase::MakeError(EterBase::EntityError::NotFound);
            if (!it->second.isMoving) return EterBase::MakeError(EterBase::EntityError::NotFound);

            auto& state = it->second;
            
            float dx = state.targetPos.x - state.currentPos.x;
            float dy = state.targetPos.y - state.currentPos.y;
            float distance = std::hypot(dx, dy);

            if (distance < 1.0f) {
                state.currentPos.x = state.targetPos.x;
                state.currentPos.y = state.targetPos.y;
                state.isMoving = false;
            } else {
                float step = state.currentPos.speed * deltaTime;
                if (step > distance) step = distance;

                state.currentPos.x += (dx / distance) * step;
                state.currentPos.y += (dy / distance) * step;
            }
            
            UserInterface::Core::EventBus::GetInstance().Publish(
                Core::ActorMovementPredictedEvent(id, state.currentPos.x, state.currentPos.y, state.currentPos.speed)
            );
            return {};
        }

        void UpdatePrediction(EterBase::EntityId id, float deltaTime) override {
            (void)UpdatePredictionModern(id, deltaTime);
        }

        EterBase::PacketResult<void> OnServerMovePacketModern(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& state = m_states[id];
            
            if (serverTime <= state.lastServerTime && state.lastServerTime != 0) {
                EterBase::ModernLogger::Debug("Ignoring outdated move packet for actor {}", id.value());
                return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
            }

            if (state.lastServerTime == 0) {
                // First packet seen for this actor, teleport to start position
                state.currentPos.x = destX;
                state.currentPos.y = destY;
            }

            state.targetPos.x = destX;
            state.targetPos.y = destY;
            state.currentPos.speed = speed;
            state.lastServerTime = serverTime;
            state.isMoving = true;
            
            EterBase::ModernLogger::Debug("Received MovePacket Actor {} -> ({}, {}), Speed: {}", id.value(), destX, destY, speed);
            return {};
        }

        void OnServerMovePacket(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) override {
            (void)OnServerMovePacketModern(id, destX, destY, speed, serverTime);
        }

        EterBase::Result<PredictedPosition, EterBase::EntityError> GetPredictedPositionModern(EterBase::EntityId id) const {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto it = m_states.find(id);
            if (it != m_states.end()) {
                return it->second.currentPos;
            }
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        PredictedPosition GetPredictedPosition(EterBase::EntityId id) const override {
            return GetPredictedPositionModern(id).value_or(PredictedPosition{});
        }

        EterBase::Result<void, EterBase::EntityError> ResetEntityModern(EterBase::EntityId id) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_states.erase(id) == 0) {
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }
            EterBase::ModernLogger::Debug("Reset jitter buffer for actor {}", id.value());
            return {};
        }

        void ResetEntity(EterBase::EntityId id) override {
            (void)ResetEntityModern(id);
        }

        void Clear() override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_states.clear();
            EterBase::ModernLogger::Info("Cleared all jitter buffers");
        }
    };

    std::unique_ptr<IMovementPredictionService> CreateMovementPredictionService() {
        return std::make_unique<ActorJitterBuffer>();
    }
}
