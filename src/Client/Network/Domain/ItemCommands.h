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
        uint32_t count{1};  ///< Liczba sztuk / zloto
    };

    /**
     * @brief Czysta komenda domenowa przemieszczenia przedmiotu w ekwipunku (DTO).
     */
    struct ItemMoveCommand
    {
        uint8_t  srcWindow{0};
        uint16_t srcCell{0};
        uint8_t  dstWindow{0};
        uint16_t dstCell{0};
        uint16_t count{1};
    };

    /**
     * @brief Czysta komenda domenowa podniesienia przedmiotu z ziemi (DTO).
     */
    struct ItemPickUpCommand
    {
        uint32_t vid{0};
    };
}

namespace Domain = Network::Domain;
