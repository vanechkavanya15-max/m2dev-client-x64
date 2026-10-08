#pragma once

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/PacketResult.h"
#include <vector>
#include <cstdint>
#include <string_view>

namespace Client::Network {

    /**
     * @class SafeboxCommandEncoder
     * @brief Koder pakietow sieciowych dla akcji w magazynie (SafeBox).
     */
    class SafeboxCommandEncoder {
    public:
        /**
         * @brief Koduje polecenie autoryzacji haslem do magazynu.
         * Tworzy pakiet czatu (/safebox_password <haslo>).
         * @param password Haslo do depozytu.
         * @return Zbudowany pakiet typu ChatCS.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodePassword(std::string_view password);

        /**
         * @brief Koduje polecenie zdeponowania lub pobrania zlota.
         * @param state Stan operacji (SAFEBOX_MONEY_STATE_SAVE lub WITHDRAW).
         * @param money Ilosc zlota.
         * @return Zbudowany pakiet TPacketCGSafeboxMoney.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeMoney(uint8_t state, int32_t money);

        /**
         * @brief Koduje polecenie deponowania przedmiotu do magazynu (Checkin).
         * @param inventorySlot Pozycja w ekwipunku postaci (zrodlo).
         * @param safeboxSlot Pozycja w depozycie (cel).
         * @return Zbudowany pakiet TPacketCGSafeboxCheckin.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeCheckin(EterBase::ItemSlot inventorySlot, uint8_t safeboxSlot);

        /**
         * @brief Koduje polecenie pobrania przedmiotu z magazynu (Checkout).
         * @param safeboxSlot Pozycja w depozycie (zrodlo).
         * @param inventorySlot Pozycja w ekwipunku postaci (cel).
         * @return Zbudowany pakiet TPacketCGSafeboxCheckout.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeCheckout(uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot);

        /**
         * @brief Koduje polecenie przeniesienia przedmiotu wewnatrz magazynu.
         * @param sourceSlot Pozycja zrodlowa w depozycie.
         * @param targetSlot Pozycja docelowa w depozycie.
         * @param count Ilosc przenoszona.
         * @return Zbudowany pakiet TPacketCGItemMove uzywajacy naglowka SAFEBOX_ITEM_MOVE.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeItemMove(uint8_t sourceSlot, uint8_t targetSlot, uint8_t count);
    };

} // namespace Client::Network
