#pragma once

#include <cstdint>

namespace Network::Domain
{
    /**
     * @brief Czysta komenda domenowa uzycia przedmiotu (DTO).
     */
    struct ItemUseCommand
    {
        uint8_t  window{0}; ///< Typ okna ekwipunku
        uint16_t cell{0};   ///< Indeks komorki
    };

    /**
     * @brief Czysta komenda domenowa upuszczenia przedmiotu (DTO).
     */
    struct ItemDropCommand
    {
        uint8_t  window{0}; ///< Typ okna ekwipunku
        uint16_t cell{0};   ///< Indeks komorki
        uint32_t count{1};  ///< Liczba sztuk
    };
}

namespace Domain = Network::Domain;
