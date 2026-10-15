#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <format>
#include "../src/EterBase/Result.h"
#include "../src/Client/Network/DynamicPacketFramer.h"

// Mock StdAfx.h inclusion for test compilation by directly including the cpp
#define MOCK_STDAFX
#include "../src/Client/Network/DynamicPacketFramer.cpp"

void TestYmir1B() {
    Client::Network::DynamicPacketFramer framer;
    framer.SetMode(Client::Network::FramingMode::Ymir1B);
    framer.RegisterExpectedSize(0x01, 5); // Opcode 1, length 5
    
    // Test Unknown Opcode
    std::vector<uint8_t> unknown_data = {0x02, 0x00, 0x00};
    framer.AppendData(unknown_data);
    auto res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::UnknownOpcode);
    framer.Clear();

    // Test Buffer Underflow
    std::vector<uint8_t> partial_data = {0x01, 0x00, 0x00};
    framer.AppendData(partial_data);
    res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::BufferUnderflow);

    // Test Valid Packet
    std::vector<uint8_t> rest_data = {0x00, 0x00};
    framer.AppendData(rest_data);
    res = framer.FrameNextPacket();
    assert(res.has_value());
    assert(res.value().size() == 5);
    assert(res.value()[0] == 0x01);
    
    // Check if buffer is empty
    res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::BufferUnderflow);
}

void TestM2Dev4B() {
    Client::Network::DynamicPacketFramer framer;
    framer.SetMode(Client::Network::FramingMode::M2Dev4B);
    
    // Test Buffer Underflow (header)
    std::vector<uint8_t> partial_header = {0x01, 0x00}; // Only opcode, no length
    framer.AppendData(partial_header);
    auto res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::BufferUnderflow);
    framer.Clear();

    // Test Invalid Header (length < 4)
    std::vector<uint8_t> invalid_header = {0x01, 0x00, 0x03, 0x00}; // Opcode 1, length 3
    framer.AppendData(invalid_header);
    res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::InvalidHeader);
    framer.Clear();

    // Test Buffer Underflow (body)
    std::vector<uint8_t> partial_body = {0x01, 0x00, 0x06, 0x00, 0xAA}; // Length 6, only 5 bytes provided
    framer.AppendData(partial_body);
    res = framer.FrameNextPacket();
    assert(!res.has_value() && res.error() == EterBase::PacketError::BufferUnderflow);

    // Test Valid Packet
    std::vector<uint8_t> rest_body = {0xBB};
    framer.AppendData(rest_body);
    res = framer.FrameNextPacket();
    assert(res.has_value());
    assert(res.value().size() == 6);
    assert(res.value()[4] == 0xAA);
    assert(res.value()[5] == 0xBB);
}

int main() {
    TestYmir1B();
    TestM2Dev4B();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
