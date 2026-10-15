#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

#include <algorithm>

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie emitowane gdy encja przekroczy granice mapy i zostanie przycieta.
     */
    struct BoundaryHitEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        float originalX;
        float originalY;
        float clampedX;
        float clampedY;

        BoundaryHitEvent(EterBase::EntityId id, float origX, float origY, float clampX, float clampY)
            : entityId(id), originalX(origX), originalY(origY), clampedX(clampX), clampedY(clampY) {}
    };

    /**
     * @brief Zapewnia ze wszystkie encje pozostaja wewnatrz granic mapy.
     * @param table Tabela komponentow transformacji encji (SoA).
     * @param minX Minimalna wartosc X.
     * @param maxX Maksymalna wartosc X.
     * @param minY Minimalna wartosc Y.
     * @param maxY Maksymalna wartosc Y.
     * @return EterBase::VoidResult Zwraca sukces lub blad domeny (np. gdy podano odwrotne granice).
     */
    [[nodiscard]] EterBase::VoidResult<EterBase::EntityError> ClampToMapBoundaries(
        TransformComponentTable& table, float minX, float maxX, float minY, float maxY)
    {
        if (minX >= maxX || minY >= maxY)
        {
            EterBase::ModernLogger::Error("ClampToMapBoundaries: Invalid boundary limits provided.");
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        size_t size = table.Size();
        for (size_t i = 0; i < size; ++i)
        {
            float posX = table.posX[i];
            float posY = table.posY[i];

            bool clamped = false;
            float newX = std::clamp(posX, minX, maxX);
            float newY = std::clamp(posY, minY, maxY);

            if (newX != posX || newY != posY)
            {
                table.posX[i] = newX;
                table.posY[i] = newY;
                clamped = true;
            }

            if (clamped)
            {
                EterBase::EntityId entityId{table.entityIds[i]};
                EterBase::ModernLogger::Debug(
                    "Entity {} clamped to boundary. Old: ({}, {}), New: ({}, {})",
                    entityId.value(), posX, posY, newX, newY);

                BoundaryHitEvent event(entityId, posX, posY, newX, newY);
                UserInterface::Core::EventBus::GetInstance().Publish(event);
            }
        }

        return {};
    }
}
