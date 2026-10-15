#pragma once

#include <cstdint>
#include <span>
#include "../../GameType.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu usuniecia przedmiotu z ekwipunku (C++20).
     */
    struct ItemDelPacket
    {
        uint16_t header;
        uint16_t length;
        TItemPos pos;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza pakiet usuniecia przedmiotu i czysci dany slot ekwipunku w pamieci.
     * @param buffer Bufor bajtow zawierajacy strukture ItemDelPacket.
     * @return Zwraca true jesli pakiet zostal przetworzony, false w przypadku bledu (np. zbyt maly bufor).
     */
    bool HandleItemDelPacket(std::span<const uint8_t> buffer);
}
