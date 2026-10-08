#pragma once

#include "../../EterBase/Result.h"
#include "../Core/DomainCommands.h"
#include <vector>
#include <cstdint>

namespace Client::Network {

    class CombatCommandEncoder {
    public:
        /**
         * @brief Koduje komendę ataku w format pakietu sieciowego.
         * @param cmd Parametry ataku.
         * @param isYmirFraming Jeżeli true, pakiet posiada 1-bajtowy nagłówek (Ymir). Jeżeli false, 4-bajtowy (m2dev).
         * @return Oczekiwany bufor bajtów, albo błąd.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeAttackCommand(
            const Client::Core::AttackCommand& cmd, 
            bool isYmirFraming);

        /**
         * @brief Koduje komendę strzału (shoot) w format pakietu sieciowego.
         * @param cmd Parametry strzału.
         * @param isYmirFraming Jeżeli true, pakiet posiada 1-bajtowy nagłówek (Ymir). Jeżeli false, 4-bajtowy (m2dev).
         * @return Oczekiwany bufor bajtów, albo błąd.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeShootCommand(
            const Client::Core::ShootCommand& cmd, 
            bool isYmirFraming);
    };

} // namespace Client::Network
