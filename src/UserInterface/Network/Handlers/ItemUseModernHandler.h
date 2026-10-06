#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu dla uzycia przedmiotu przez serwer GC (TPacketGCItemUse).
     */
    struct PacketItemUse
    {
        uint16_t header;
        uint16_t length;
        uint8_t window_type;
        uint16_t cell;
        uint32_t ch_vid;
        uint32_t victim_vid;
        uint32_t vnum;
    };
#pragma pack(pop)

    /**
     * @brief Zdarzenie rozglaszane po udanym uzyciu przedmiotu wymagajace odswiezenia okna inwentarza.
     */
    struct InventoryRefreshEvent : public UserInterface::Core::IEvent
    {
        /**
         * @brief Domyslny konstruktor.
         */
        InventoryRefreshEvent() = default;
    };

    /**
     * @brief Przetwarza pakiet uzycia przedmiotu (mikstura, ksiega, zwój) i powiadamia EventBus C++23.
     * @param buffer Bufor bajtow pakietu GC.
     * @return EterBase::PacketResult<void> wskazujacy sukces lub kod bledu (np. BufferUnderflow).
     */
    EterBase::PacketResult<void> ProcessItemUsePacket(std::span<const uint8_t> buffer);
}
