#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <cmath>
#include <vector>
#include <expected>
#include <cstdint>

namespace UserInterface::ECS
{

    /**
     * @brief Zdarzenie emitowane gdy encja znajdzie sie w okreslonym zasiegu gracza.
     */
    struct EntityInRangeEvent : public Core::IEvent
    {
        uint32_t entityId;
        float distance;

        EntityInRangeEvent(uint32_t id, float dist) : entityId(id), distance(dist) {}
    };

    /**
     * @brief Usługa obliczania dystansu dla komponentów transformacji.
     */
    class TransformDistanceService final
    {
    public:
        /**
         * @brief Oblicza odległości między podanym bytem (np. graczem) a wszystkimi innymi bytami
         * w podanej tablicy, wykorzystując optymalizacje wektoryzacji.
         * 
         * @param table Tablica komponentów (SoA)
         * @param sourceId Identyfikator bytu źródłowego
         * @param threshold Odległość progowa, powyżej (lub poniżej) której generowane jest zdarzenie.
         * @return std::expected<std::vector<float>, EterBase::EntityError> 
         */
        static std::expected<std::vector<float>, EterBase::EntityError> CalculateDistances(
            const TransformComponentTable& table, 
            EterBase::EntityId sourceId,
            float threshold)
        {
            size_t sourceIdx = -1;
            bool found = false;

            // Szukanie indeksu bytu źródłowego
            for (size_t i = 0; i < table.Size(); ++i)
            {
                if (table.entityIds[i] == sourceId.value())
                {
                    sourceIdx = i;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                EterBase::ModernLogger::Warning("TransformDistanceService: Source entity {} not found", sourceId.value());
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            float srcX = table.posX[sourceIdx];
            float srcY = table.posY[sourceIdx];
            float srcZ = table.posZ[sourceIdx];

            size_t count = table.Size();
            std::vector<float> distances(count);
            
            // Wektoryzowana petla obliczania dystansu (SIMD auto-vectorization friendly)
            for (size_t i = 0; i < count; ++i)
            {
                float dx = table.posX[i] - srcX;
                float dy = table.posY[i] - srcY;
                float dz = table.posZ[i] - srcZ;

                distances[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
            }

            // Druga pętla dla wysyłania zdarzeń (żeby nie psuć wektoryzacji pierwszej)
            for (size_t i = 0; i < count; ++i)
            {
                if (i != sourceIdx && distances[i] <= threshold)
                {
                    Core::EventBus::GetInstance().Publish(EntityInRangeEvent(table.entityIds[i], distances[i]));
                    EterBase::ModernLogger::Trace("Entity {} is in range ({})", table.entityIds[i], distances[i]);
                }
            }

            EterBase::ModernLogger::Info("TransformDistanceService: Calculated distances for {} entities", count);
            return distances;
        }
    };

} // namespace UserInterface::ECS
