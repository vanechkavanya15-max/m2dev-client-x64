#include <iostream>
#include <cassert>
#include <vector>
#include <span>
#include <cstring>

#define _MOCK_PACKET_H
#include "src/Client/Network/ExchangePacketCodec.h"

using namespace Client::Network;

void TestEncodeExchangeStart() {
    auto buffer = ExchangePacketCodec::EncodeExchangeStart(12345);
    assert(buffer.size() == sizeof(TPacketCGExchange));
    
    TPacketCGExchange packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGExchange));
    
    assert(packet.header == CG::EXCHANGE);
    assert(packet.length == sizeof(TPacketCGExchange));
    assert(packet.subheader == ExchangeSub::CG::START);
    assert(packet.arg1 == 12345);
}

void TestEncodeExchangeItemAdd() {
    TItemPos itemPos{1, 5};
    auto buffer = ExchangePacketCodec::EncodeExchangeItemAdd(2, itemPos);
    assert(buffer.size() == sizeof(TPacketCGExchange));
    
    TPacketCGExchange packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGExchange));
    
    assert(packet.header == CG::EXCHANGE);
    assert(packet.length == sizeof(TPacketCGExchange));
    assert(packet.subheader == ExchangeSub::CG::ITEM_ADD);
    assert(packet.arg2 == 2);
    assert(packet.Pos.window_type == 1);
    assert(packet.Pos.cell == 5);
}

void TestEncodeExchangeElkAdd() {
    auto buffer = ExchangePacketCodec::EncodeExchangeElkAdd(1000);
    assert(buffer.size() == sizeof(TPacketCGExchange));
    
    TPacketCGExchange packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGExchange));
    
    assert(packet.header == CG::EXCHANGE);
    assert(packet.length == sizeof(TPacketCGExchange));
    assert(packet.subheader == ExchangeSub::CG::ELK_ADD);
    assert(packet.arg1 == 1000);
}

void TestEncodeExchangeAccept() {
    auto buffer = ExchangePacketCodec::EncodeExchangeAccept();
    assert(buffer.size() == sizeof(TPacketCGExchange));
    
    TPacketCGExchange packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGExchange));
    
    assert(packet.header == CG::EXCHANGE);
    assert(packet.length == sizeof(TPacketCGExchange));
    assert(packet.subheader == ExchangeSub::CG::ACCEPT);
}

void TestEncodeExchangeCancel() {
    auto buffer = ExchangePacketCodec::EncodeExchangeCancel();
    assert(buffer.size() == sizeof(TPacketCGExchange));
    
    TPacketCGExchange packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGExchange));
    
    assert(packet.header == CG::EXCHANGE);
    assert(packet.length == sizeof(TPacketCGExchange));
    assert(packet.subheader == ExchangeSub::CG::CANCEL);
}

void TestDecodeExchangePacket() {
    TPacketGCExchange sourcePacket{};
    sourcePacket.header = GC::EXCHANGE;
    sourcePacket.length = sizeof(TPacketGCExchange);
    sourcePacket.subheader = ExchangeSub::GC::START;
    sourcePacket.arg1 = 555;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCExchange));
    std::memcpy(buffer.data(), &sourcePacket, sizeof(TPacketGCExchange));
    
    auto result = ExchangePacketCodec::DecodeExchangePacket(std::span<const uint8_t>(buffer));
    assert(result.has_value());
    assert(result->header == GC::EXCHANGE);
    assert(result->length == sizeof(TPacketGCExchange));
    assert(result->subheader == ExchangeSub::GC::START);
    assert(result->arg1 == 555);
    
    // Test buffer too small
    std::vector<uint8_t> smallBuffer(sizeof(TPacketGCExchange) - 1);
    auto smallResult = ExchangePacketCodec::DecodeExchangePacket(std::span<const uint8_t>(smallBuffer));
    assert(!smallResult.has_value());
    assert(smallResult.error() == PacketError::BufferTooSmall);

    // Test invalid header
    sourcePacket.header = 0x9999;
    std::memcpy(buffer.data(), &sourcePacket, sizeof(TPacketGCExchange));
    auto invalidHeaderResult = ExchangePacketCodec::DecodeExchangePacket(std::span<const uint8_t>(buffer));
    assert(!invalidHeaderResult.has_value());
    assert(invalidHeaderResult.error() == PacketError::InvalidHeader);
}

int main() {
    TestEncodeExchangeStart();
    TestEncodeExchangeItemAdd();
    TestEncodeExchangeElkAdd();
    TestEncodeExchangeAccept();
    TestEncodeExchangeCancel();
    TestDecodeExchangePacket();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
