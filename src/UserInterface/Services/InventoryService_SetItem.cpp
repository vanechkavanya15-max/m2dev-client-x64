#include "../StdAfx.h"
#include "IInventoryService.h"
#include "InventoryService.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Domain/ItemValidator.h"

namespace UserInterface::Core
{
    /**
     * @brief Zdarzenie rozglaszane po pomyslnym ustawieniu przedmiotu na slocie.
     */
    struct InventoryItemSetEvent : public IEvent
    {
        EterBase::ItemSlot slot;
        EterBase::ItemVnum vnum;
        uint32_t count;

        InventoryItemSetEvent(EterBase::ItemSlot s, EterBase::ItemVnum v, uint32_t c)
            : slot(s), vnum(v), count(c) {}
    };
}

namespace UserInterface::Services
{
    /**
     * @brief Ustawia przedmiot na wybranym slocie w ekwipunku po weryfikacji poprawnosci slota i vnum.
     *
     * @param slot Silnie typowany slot, na ktorym przedmiot ma zostac ustawiony.
     * @param item Widok przedmiotu zawierajacy jego parametry.
     * @return EterBase::PacketResult<void> Zwraca blad jesli slot jest poza zasiegiem, inaczej sukces.
     */
    EterBase::PacketResult<void> InventoryService::SetItem(EterBase::ItemSlot slot, const InventoryItemView& item)
    {
        if (!Domain::ItemValidator::IsValidCell(INVENTORY, slot.value()))
        {
            EterBase::ModernLogger::Error("InventoryService::SetItem - Zly slot: {}", slot.value());
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (!item.vnum)
        {
            EterBase::ModernLogger::Warning("InventoryService::SetItem - Proba ustawienia VNUM = 0 na slocie: {}", slot.value());
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        // Zakladamy, ze klasa InventoryService w swoim naglowku posiada m_items np. jako tablice lub mape
        m_items[slot.value()] = item;

        EterBase::ModernLogger::Info("InventoryService::SetItem - Pomyslnie ustawiono przedmiot. VNUM: {}, Ilosc: {}, Slot: {}", 
                                     item.vnum.value(), item.count, slot.value());

        Core::EventBus::GetInstance().Publish(Core::InventoryItemSetEvent{slot, item.vnum, item.count});

        return {};
    }
}
