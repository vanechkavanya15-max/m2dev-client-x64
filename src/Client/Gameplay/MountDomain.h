#pragma once

#include <memory>
#include <expected>
#include <string>
#include <cstdint>

namespace Client::Gameplay
{
    /**
     * @brief Interfejs domeny zarzadzajacej wierzchowcami. (C++23 Zero-Conflict)
     */
    class IMountDomain
    {
    public:
        virtual ~IMountDomain() = default;

        /**
         * @brief Rozpoczyna jazde konna na wierzchowcu o podanym VNUM.
         */
        virtual std::expected<void, std::string> Mount(uint32_t mountVnum) = 0;

        /**
         * @brief Konczy jazde konna.
         */
        virtual std::expected<void, std::string> Dismount() = 0;

        /**
         * @brief Sprawdza czy gracz aktualnie jedzie na wierzchowcu.
         */
        [[nodiscard]] virtual bool IsMounting() const = 0;

        /**
         * @brief Pobiera poziom wierzchowca na podstawie jego VNUM (1, 2, 3...).
         */
        [[nodiscard]] virtual uint32_t GetMountLevel() const = 0;

        /**
         * @brief Zwraca bonus do predkosci poruszania sie na podstawie poziomu.
         */
        [[nodiscard]] virtual uint32_t GetSpeedBonus() const = 0;

        /**
         * @brief Weryfikuje czy postac moze wejsc w podany tryb ruchu.
         */
        [[nodiscard]] virtual std::expected<void, std::string> CanChangeMotionMode(uint32_t newMode) const = 0;
    };

    /**
     * @brief Fabryka instancji IMountDomain.
     */
    std::unique_ptr<IMountDomain> CreateMountDomain();

} // namespace Client::Gameplay
