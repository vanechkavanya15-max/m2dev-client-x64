#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <expected>

#include "../../UserInterface/Packet.h"

#include "EterBase/Result.h"

namespace Client::Network {

    using EterBase::PacketError;
    using EterBase::PacketResult;

    class ExchangePacketCodec {
    public:
        static PacketResult<TPacketGCExchange> DecodeExchangePacket(std::span<const uint8_t> buffer);
        
        static std::vector<uint8_t> EncodeExchangeStart(uint32_t targetVid);
        static std::vector<uint8_t> EncodeExchangeItemAdd(uint8_t displayPos, TItemPos itemPos);
        static std::vector<uint8_t> EncodeExchangeElkAdd(uint32_t amount);
        static std::vector<uint8_t> EncodeExchangeAccept();
        static std::vector<uint8_t> EncodeExchangeCancel();
    };

} // namespace Client::Network
