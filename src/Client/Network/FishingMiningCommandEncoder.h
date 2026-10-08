#pragma once

#include "../../EterBase/Result.h"
#include "../Core/StrongTypes.h"
#include <vector>
#include <cstdint>

namespace Client::Network {

    /**
     * @class FishingMiningCommandEncoder
     * @brief Koder pakietów dla operacji wędkarskich (Fishing) oraz wydobywczych (Mining/Ore).
     * Zgodny ze standardem Zero-Conflict MSVC C++23.
     */
    class FishingMiningCommandEncoder {
    public:
        /**
         * @brief Koduje komendę zarzucenia wędki.
         * @param rotationAngle Kąt rotacji gracza (np. w stopniach).
         * @return Zbuforowany pakiet gotowy do wysłania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeCastFishingRod(float rotationAngle);

        /**
         * @brief Koduje komendę wyciągnięcia wędki (ze statusem sukcesu/porażki).
         * @param isSuccess Flaga określająca czy wyciągnięcie zakończyło się sukcesem.
         * @return Zbuforowany pakiet gotowy do wysłania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeReelFishingRod(bool isSuccess);

        /**
         * @brief Koduje komendę rozpoczęcia wydobycia na żyle mineralnej (Mining).
         * @param veinVid Identyfikator żylaku (EntityVid).
         * @return Zbuforowany pakiet gotowy do wysłania.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeMiningAction(Client::Core::EntityVid veinVid);
    };

} // namespace Client::Network
