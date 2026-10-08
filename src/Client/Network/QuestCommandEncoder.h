#pragma once

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/PacketResult.h"
#include <vector>
#include <cstdint>

namespace Client::Network {

    /**
     * @brief Koder dla komend questow oraz klikniec w obiekty (Zero-Conflict Standard 2026).
     */
    class QuestCommandEncoder {
    public:
        /**
         * @brief Koduje pakiet odpowiedzi w oknie questa (CG_SCRIPT_ANSWER).
         * @param answer Wybrany indeks odpowiedzi w dialogu.
         * @return Bufor zawierajacy zakodowany pakiet TPacketCGScriptAnswer.
         */
        static EterBase::PacketResult<std::vector<uint8_t>> EncodeScriptAnswer(uint8_t answer);

        /**
         * @brief Koduje pakiet klikniecia w cel (CG_ON_CLICK).
         * @param targetId Typ silny (EntityId) docelowego obiektu (VID) kliknietego przez gracza.
         * @return Bufor zawierajacy zakodowany pakiet TPacketCGOnClick.
         */
        static EterBase::PacketResult<std::vector<uint8_t>> EncodeOnClick(EterBase::EntityId targetId);
    };

} // namespace Client::Network
