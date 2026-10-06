#pragma once

#include "IInstanceEquipmentModelController.h"
#include "../Core/EventBus.h"
#include <array>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie emitowane przy zmianie elementu ekwipunku postaci (np. tors/zbroja).
     */
    struct EquipmentPartChangedEvent : public Core::IEvent {
        ModelPart part;
        EterBase::ItemVnum vnum;

        EquipmentPartChangedEvent(ModelPart part, EterBase::ItemVnum vnum)
            : part(part), vnum(vnum) {}
    };

    /**
     * @brief Kontroler modelu ekwipunku instancji postaci (C++23 SRP).
     * 
     * Zarzadza podmiana poszczegolnych elementow modelu 3D (np. zbroja/tors, bron).
     * Calkowicie odsprzezony od GUI, wykorzystuje EventBus.
     */
    class InstanceEquip_Body : public IInstanceEquipmentModelController
    {
    public:
        InstanceEquip_Body();
        ~InstanceEquip_Body() override;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        std::array<EterBase::ItemVnum, static_cast<size_t>(ModelPart::MaxParts)> parts_;
        uint8_t lodLevel_;
    };
}
