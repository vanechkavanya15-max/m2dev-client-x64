#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Naglowek pakietu odebranej wiadomosci czatu (TPacketGCChat).
     */
    struct PacketChatHeader
    {
        uint16_t header;
        uint16_t length;
        uint8_t  type;
        uint32_t senderId;
        uint8_t  empire;
    };
#pragma pack(pop)

    /**
     * @brief Przetwarza wiadomosc czatu zwracajac PacketResult C++23.
     * @param buffer Pelny bufor strumienia wiadomosci czatu.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessChatMessage(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza odebrana wiadomosc czatu i kieruje ja do bufora logiki.
     * @param buffer Pelny bufor strumienia wiadomosci czatu.
     * @return true w przypadku powodzenia.
     */
    bool HandleChatMessage(std::span<const uint8_t> buffer);
}
