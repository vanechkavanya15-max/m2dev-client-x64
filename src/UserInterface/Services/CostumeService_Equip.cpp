#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Services
{
    struct CostumeEquippedEvent : public Core::IEvent
    {
        EterBase::ItemSlot slot;
        EterBase::ItemVnum vnum;
        bool isEquipped;

        CostumeEquippedEvent(EterBase::ItemSlot s, EterBase::ItemVnum v, bool equipped)
            : slot(s), vnum(v), isEquipped(equipped) {}
    };

    class CostumeServiceEquip
    {
    public:
        static EterBase::PacketResult<void> EquipCostume(EterBase::ItemSlot slot, EterBase::ItemVnum vnum)
        {
            if (vnum.value() == 0)
            {
                EterBase::ModernLogger::Error("CostumeServiceEquip: Invalid costume item vnum 0");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            Core::EventBus::GetInstance().Publish(CostumeEquippedEvent(slot, vnum, true));
            EterBase::ModernLogger::Info("CostumeServiceEquip: Equipped costume vnum {} to slot {}", vnum.value(), slot.value());
            return {};
        }

        static EterBase::PacketResult<void> UnequipCostume(EterBase::ItemSlot slot, EterBase::ItemVnum vnum)
        {
            Core::EventBus::GetInstance().Publish(CostumeEquippedEvent(slot, vnum, false));
            EterBase::ModernLogger::Info("CostumeServiceEquip: Unequipped costume from slot {}", slot.value());
            return {};
        }
    };
}
