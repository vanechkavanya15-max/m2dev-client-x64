#include "ExchangePacketCodec.h"
#include <cstring>
#include <iostream>

namespace Client::Network {

    PacketResult<TPacketGCExchange> ExchangePacketCodec::DecodeExchangePacket(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCExchange)) {
            return std::unexpected(PacketError::BufferUnderflow);
        }

        TPacketGCExchange packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCExchange));

        if (packet.header != GC::EXCHANGE) {
            return std::unexpected(PacketError::InvalidHeader);
        }

        return packet;
    }

    std::vector<uint8_t> ExchangePacketCodec::EncodeExchangeStart(uint32_t targetVid) {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::START;
        packet.arg1 = targetVid;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));
        return buffer;
    }

    std::vector<uint8_t> ExchangePacketCodec::EncodeExchangeItemAdd(uint8_t displayPos, TItemPos itemPos) {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::ITEM_ADD;
        packet.arg2 = displayPos;
        packet.Pos = itemPos;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));
        return buffer;
    }

    std::vector<uint8_t> ExchangePacketCodec::EncodeExchangeElkAdd(uint32_t amount) {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::ELK_ADD;
        packet.arg1 = amount;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));
        return buffer;
    }

    std::vector<uint8_t> ExchangePacketCodec::EncodeExchangeAccept() {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::ACCEPT;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));
        return buffer;
    }

    std::vector<uint8_t> ExchangePacketCodec::EncodeExchangeCancel() {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::CANCEL;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));
        return buffer;
    }

} // namespace Client::Network
