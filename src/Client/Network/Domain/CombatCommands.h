#pragma once

#include <cstdint>
#include <optional>

namespace Network::Domain
{
    /**
     * @brief Czysta komenda domenowa ataku (DTO, niezalezna od protokolu sieciowego).
     */
    struct AttackCommand
    {
        uint32_t targetVid{0};       ///< Identyfikator celu / ofiary (dwVictimVID)
        uint32_t attackerVid{0};     //< Identyfikator atakujacego
        uint8_t  attackType{0};      ///< Typ ataku (np. 0 = standardowy)
        uint16_t sequence{0};        ///< Sekwencja synchronizacji / CRC
        uint32_t attackMotion{0};    ///< Indeks motywu / combo (jesli dotyczy)
    };

    /**
     * @brief Zdarzenie domenowe otrzymanych obrazen (DTO).
     */
    struct DamageInfoEvent
    {
        uint32_t victimVid{0};
        uint32_t attackerVid{0};
        uint32_t damage{0};
        uint8_t  flag{0};
    };
}

namespace Domain = Network::Domain;
