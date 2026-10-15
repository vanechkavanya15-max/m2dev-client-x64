#include "../../StdAfx.h"
#include "IMovementPredictionService.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"
#include "UserInterface/Core/MovementEvents.h"

#include <unordered_map>
#include <cmath>
#include <memory>

namespace UserInterface::Network
{
    /**
     * @brief Zdarzenie emitowane po wykryciu anomalii predkosci (teleportacji).
     */
    struct SpeedAnomalyDetectedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        float distance;
        float maxDistance;
        float timeDiffSeconds;

        SpeedAnomalyDetectedEvent(EterBase::EntityId id, float dist, float maxDist, float timeDiff)
            : entityId(id), distance(dist), maxDistance(maxDist), timeDiffSeconds(timeDiff) {}
    };

    /**
     * @brief Weryfikacja anomalii predkosci encji (anty-teleport).
     * 
     * Implementacja IMovementPredictionService, ktora nadzoruje odleglosci miedzy 
     * kolejnymi pakietami ruchu od serwera i wykrywa desynchronizacje / teleporty.
     */
    class ActorSpeedVerifier final : public IMovementPredictionService
    {
    public:
        ActorSpeedVerifier() = default;
        ~ActorSpeedVerifier() override = default;

        /**
         * @brief Aktualizuje przewidywana pozycje encji.
         * @param id Identyfikator encji (EntityId).
         * @param deltaTime Czas od ostatniej aktualizacji w sekundach.
         */
        void UpdatePrediction(EterBase::EntityId id, float deltaTime) override
        {
            auto it = m_entities.find(id);
            if (it == m_entities.end())
            {
                return;
            }

            // Simple movement prediction could be applied here if needed.
            // For anti-teleport verification, we mostly rely on OnServerMovePacket.
        }

        /**
         * @brief Przetwarza ruch przysłany z serwera i weryfikuje poprawność (brak anomalii).
         * @param id Identyfikator encji.
         * @param destX Docelowa współrzędna X.
         * @param destY Docelowa współrzędna Y.
         * @param speed Prędkość encji.
         * @param serverTime Czas po stronie serwera (ms).
         */
        void OnServerMovePacket(EterBase::EntityId id, float destX, float destY, float speed, uint32_t serverTime) override
        {
            auto it = m_entities.find(id);
            if (it == m_entities.end())
            {
                PredictedPosition pos{};
                pos.x = destX;
                pos.y = destY;
                pos.z = 0.0f;
                pos.yaw = 0.0f;
                pos.speed = speed;
                
                m_entities[id] = pos;
                m_lastServerTimes[id] = serverTime;
                
                EterBase::ModernLogger::Debug("ActorSpeedVerifier: Initialize entity {} at ({}, {})", id.value(), destX, destY);
                return;
            }

            PredictedPosition& currentPos = it->second;
            uint32_t lastTime = m_lastServerTimes[id];
            
            float dx = destX - currentPos.x;
            float dy = destY - currentPos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Wylicz roznice czasu (z uwzglednieniem zawiniecia licznika uint32_t)
            float timeDiffSeconds = 0.0f;
            if (serverTime >= lastTime)
            {
                timeDiffSeconds = static_cast<float>(serverTime - lastTime) / 1000.0f;
            }
            else
            {
                timeDiffSeconds = static_cast<float>((UINT32_MAX - lastTime) + serverTime + 1) / 1000.0f;
            }

            // Maksymalny dopuszczalny dystans z uwzglednieniem predkosci, czasu i bezpiecznego marginesu na opoznienia
            float maxAcceptableDistance = (speed * timeDiffSeconds) + 500.0f;

            if (distance > maxAcceptableDistance && timeDiffSeconds > 0.0f)
            {
                EterBase::ModernLogger::Warning("ActorSpeedVerifier: Speed anomaly (teleport) detected for entity {}! Dist: {} > Max: {}, TimeDiff: {}s", 
                    id.value(), distance, maxAcceptableDistance, timeDiffSeconds);
                
                SpeedAnomalyDetectedEvent anomalyEvent{id, distance, maxAcceptableDistance, timeDiffSeconds};
                UserInterface::Core::EventBus::GetInstance().Publish(anomalyEvent);
                
                // Wymuszenie aktualizacji pozycji na serwerowa
                currentPos.x = destX;
                currentPos.y = destY;
                currentPos.speed = speed;
                m_lastServerTimes[id] = serverTime;
            }
            else
            {
                currentPos.x = destX;
                currentPos.y = destY;
                currentPos.speed = speed;
                m_lastServerTimes[id] = serverTime;
            }
        }

        /**
         * @brief Pobiera aktualna przewidywana pozycje.
         * @param id Identyfikator encji.
         * @return Struktura z wyestymowana pozycja i predkoscia.
         */
        [[nodiscard]] PredictedPosition GetPredictedPosition(EterBase::EntityId id) const override
        {
            auto it = m_entities.find(id);
            if (it != m_entities.end())
            {
                return it->second;
            }
            return PredictedPosition{};
        }

        /**
         * @brief Cofa/resetuje stan wybranej encji.
         * @param id Identyfikator encji.
         */
        void ResetEntity(EterBase::EntityId id) override
        {
            if (m_entities.erase(id))
            {
                m_lastServerTimes.erase(id);
                EterBase::ModernLogger::Debug("ActorSpeedVerifier: Reset entity {}.", id.value());
            }
        }

        /**
         * @brief Cofa stan dla wszystkich encji.
         */
        void Clear() override
        {
            m_entities.clear();
            m_lastServerTimes.clear();
            EterBase::ModernLogger::Info("ActorSpeedVerifier: Cleared all entities.");
        }

    private:
        std::unordered_map<EterBase::EntityId, PredictedPosition> m_entities;
        std::unordered_map<EterBase::EntityId, uint32_t> m_lastServerTimes;
    };
    
    // Factory function to instantiate the service.
    std::unique_ptr<IMovementPredictionService> CreateSpeedVerifierMovementPredictionService()
    {
        return std::make_unique<ActorSpeedVerifier>();
    }
}
