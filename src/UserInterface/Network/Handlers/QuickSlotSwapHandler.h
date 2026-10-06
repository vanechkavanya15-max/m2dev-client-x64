#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu zamiany miejscami slotow (TPacketGCQuickSlotSwap).
     */
    struct PacketQuickSlotSwap
    {
        uint16_t header;
        uint16_t length;
        uint8_t pos;
        uint8_t change_pos;
    };
#pragma pack(pop)

    /**
     * @brief Zdarzenie wywolywane po udanej zamianie miejscami quickslotow.
     */
    struct QuickSlotSwapEvent : public UserInterface::Core::IEvent
    {
        uint8_t pos{0};
        uint8_t change_pos{0};

        QuickSlotSwapEvent() = default;
        QuickSlotSwapEvent(uint8_t p, uint8_t cp) : pos(p), change_pos(cp) {}
    };

    /**
     * @brief Przetwarza zamiane quickslotow zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessQuickSlotSwap(std::span<const uint8_t> buffer);

    /**
     * @brief Kompatybilny z C++ wrapper obslugujacy zamiane quickslotow.
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego zdekodowania pakietu i zmiany w pamieci.
     */
    bool HandleQuickSlotSwapPacket(std::span<const uint8_t> buffer);
}
