#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Core/EventBus.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include <cmath>
#include <span>

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie aktualizacji pozycji wyemitowane dla zaktualizowanej encji.
     */
    struct TransformUpdatedEvent
    {
        EterBase::EntityId entityId;
        float x;
        float y;
        float z;
        float rot;
    };

    /**
     * @brief Wektorowa aktualizacja pozycji (Batch Update) dzialajaca na 64-bajtowych blokach.
     * 
     * Wykorzystuje strukture SoA (Structure of Arrays) do zoptymalizowanej zmiany pozycji
     * w oparciu o wektor predkosci, interpolujac do targetX i targetY.
     * 
     * @param table Referencja do SoA pozycji.
     * @param deltaTime Czas w sekundach od ostatniej klatki.
     * @return EterBase::PacketResult<void> Zwraca sukces (PacketResult::value).
     */
    EterBase::PacketResult<void> UpdateTransformsBatch(TransformComponentTable& table, float deltaTime) noexcept
    {
        if (table.Size() == 0 || deltaTime <= 0.0f)
            return {};

        const size_t count = table.Size();
        
        // Poniewaz standardowe klastrowanie 64-bajtowe (16 floats * 4 bajty) daje najlepsza wektoryzacje.
        // Wykonujemy unroll na 16.
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            for (size_t j = 0; j < 16; ++j)
            {
                size_t idx = i + j;
                float tx = table.targetX[idx];
                float ty = table.targetY[idx];
                float px = table.posX[idx];
                float py = table.posY[idx];
                float v = table.velocity[idx];

                float dx = tx - px;
                float dy = ty - py;
                float distSq = dx * dx + dy * dy;

                if (distSq > 0.01f)
                {
                    float dist = std::sqrt(distSq);
                    float moveDist = v * deltaTime;

                    if (moveDist >= dist)
                    {
                        table.posX[idx] = tx;
                        table.posY[idx] = ty;
                    }
                    else
                    {
                        table.posX[idx] = px + (dx / dist) * moveDist;
                        table.posY[idx] = py + (dy / dist) * moveDist;
                    }

                    // Emituj event aktualizacji przez EventBus
                    UserInterface::Core::EventBus::GetInstance().Publish(
                        TransformUpdatedEvent{
                            EterBase::EntityId{table.entityIds[idx]},
                            table.posX[idx],
                            table.posY[idx],
                            table.posZ[idx],
                            table.rotation[idx]
                        }
                    );
                }
            }
        }

        // Obsluga reszty encji (nie stanowiacych pelnego bloku 16)
        for (; i < count; ++i)
        {
            float tx = table.targetX[i];
            float ty = table.targetY[i];
            float px = table.posX[i];
            float py = table.posY[i];
            float v = table.velocity[i];

            float dx = tx - px;
            float dy = ty - py;
            float distSq = dx * dx + dy * dy;

            if (distSq > 0.01f)
            {
                float dist = std::sqrt(distSq);
                float moveDist = v * deltaTime;

                if (moveDist >= dist)
                {
                    table.posX[i] = tx;
                    table.posY[i] = ty;
                }
                else
                {
                    table.posX[i] = px + (dx / dist) * moveDist;
                    table.posY[i] = py + (dy / dist) * moveDist;
                }

                // Emituj event aktualizacji przez EventBus
                UserInterface::Core::EventBus::GetInstance().Publish(
                    TransformUpdatedEvent{
                        EterBase::EntityId{table.entityIds[i]},
                        table.posX[i],
                        table.posY[i],
                        table.posZ[i],
                        table.rotation[i]
                    }
                );
            }
        }

        return {};
    }
}
