#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Network::Domain
{
    /**
     * @brief Komenda wyboru krolestwa.
     */
    struct EmpireSelectCommand
    {
        uint8_t empireId{0};
    };

    /**
     * @brief Komenda wyboru postaci ze slotu.
     */
    struct CharacterSelectCommand
    {
        uint8_t slotIndex{0};
    };

    /**
     * @brief Komenda usuniecia postaci kodem prywatnym (PIN).
     */
    struct CharacterDestroyCommand
    {
        uint8_t slotIndex{0};
        std::string privateCode;
    };

    /**
     * @brief Komenda tworzenia nowej postaci.
     */
    struct CharacterCreateCommand
    {
        uint8_t slotIndex{0};
        std::string name;
        uint16_t job{0};
        uint8_t shape{0};
        uint8_t con{0};
        uint8_t int_{0};
        uint8_t str{0};
        uint8_t dex{0};
    };
}

namespace Domain = Network::Domain;
