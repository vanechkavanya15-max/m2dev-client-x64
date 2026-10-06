#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonShop.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../Handlers/ShopUpdateItemHandler.h"

#include <cstdint>
#include <span>

namespace Network::Dispatchers
{
    /**
     * @brief Aktualizacja informacji o przedmiocie w sklepie.
     * 
     * Nowoczesny handler zgodny ze standardem C++23. Weryfikuje bufor, wyodrebnia
     * pakiety strukturalne, updatuje stan w pamieci (CPythonShop) i propaguje event.
     *
     * @param buffer Skonwertowany i wyrównany zrzut pamięci binarnej.
     * @return EterBase::PacketResult<void> Pusty stan sukcesu lub odpowiedni PacketError.
     */
    EterBase::PacketResult<void> DispatchShopUpdateItem(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCShopUpdateItem))
        {
            EterBase::ModernLogger::Error("DispatchShopUpdateItem: Buffer underflow (expected {}, got {})", 
                sizeof(TPacketGCShopUpdateItem), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCShopUpdateItem*>(buffer.data());
        
        // Zabezpieczenie typu silnego (Strong Type)
        const EterBase::ItemSlot position(packet->pos);

        // Aktualizacja stanu pamieci z wykorzystaniem CPythonShop (Zero-Allocation)
        CPythonShop::Instance().SetItemData(position.value(), packet->item);

        EterBase::ModernLogger::Info("DispatchShopUpdateItem: Successfully updated item state at slot {}", position.value());
        EterBase::ModernLogger::Debug("DispatchShopUpdateItem: Vnum: {}, Price: {}", packet->item.vnum, packet->item.price);

        // Powiadomienie warstwy UI
        UserInterface::Core::EventBus::GetInstance().Publish(
            Network::Handlers::ShopItemUpdatedEvent(position)
        );

        return {};
    }
}
