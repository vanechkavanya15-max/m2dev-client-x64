#include "../StdAfx.h"
#include <cstdint>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "IInstanceEquipmentModelController.h"

namespace
{
    /**
     * @brief Zdarzenie emitowane po zmianie poziomu szczegolowosci (LOD) siatki modelu ekwipunku
     */
    struct EquipmentLODChangedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        uint8_t lodLevel;

        EquipmentLODChangedEvent(EterBase::EntityId id, uint8_t level)
            : entityId(id), lodLevel(level) {}
    };
}

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Aktualizuje poziom LOD modelu ekwipunku dla instancji i rozglasza zdarzenie po EventBus.
     * 
     * @param entityId Identyfikator podmiotu (np. gracz, mob) ktorego dotyczy zmiana
     * @param controller Referencja do kontrolera modelu sprzetu
     * @param lodLevel Nowy poziom LOD (0 = najnizszy szczegol, wyzsze = lepsze detale, zaleznie od implementacji renderera)
     * @return EterBase::PacketResult<void> Zwraca sukces (std::expected).
     */
    EterBase::PacketResult<void> UpdateEquipmentLODMesh(EterBase::EntityId entityId, IInstanceEquipmentModelController& controller, uint8_t lodLevel)
    {
        if (entityId.value() == 0)
        {
            EterBase::ModernLogger::Error("UpdateEquipmentLODMesh called with invalid EntityId: 0");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Debug("UpdateEquipmentLODMesh: Updating LOD for EntityId {} to level {}", entityId.value(), lodLevel);

        controller.SetLODLevel(lodLevel);

        UserInterface::Core::EventBus::GetInstance().Publish(EquipmentLODChangedEvent{entityId, lodLevel});

        return {};
    }
}
