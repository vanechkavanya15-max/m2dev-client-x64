#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu usuniecia przedmiotu lezacego na ziemi (TPacketGCItemGroundDel).
     */
    struct PacketItemGroundDel
    {
        uint16_t header;
        uint16_t length;
        uint32_t itemVid;
    };
#pragma pack(pop)

    /**
     * @brief Usuwa przedmiot z ziemi zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessItemGroundDel(std::span<const uint8_t> buffer);

    /**
     * @brief Usuwa przedmiot z ziemi po podniesieniu lub zniknieciu (interfejs kompatybilny).
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego usuniecia encji.
     */
    bool HandleItemGroundDel(std::span<const uint8_t> buffer);
}
