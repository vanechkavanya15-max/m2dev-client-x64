#include "../src/Client/Network/QuestCommandEncoder.h"
#include "../src/UserInterface/Packet.h"
#include <iostream>
#include <cstring>
#include <cassert>

using namespace Client::Network;

void TestEncodeScriptAnswer() {
    uint8_t expectedAnswer = 42;
    auto result = QuestCommandEncoder::EncodeScriptAnswer(expectedAnswer);
    assert(result.has_value());

    const auto& buffer = result.value();
    assert(buffer.size() == sizeof(TPacketCGScriptAnswer));

    TPacketCGScriptAnswer decodedPacket;
    std::memcpy(&decodedPacket, buffer.data(), sizeof(decodedPacket));

    assert(decodedPacket.header == CG::SCRIPT_ANSWER);
    assert(decodedPacket.length == sizeof(TPacketCGScriptAnswer));
    assert(decodedPacket.answer == expectedAnswer);

    std::cout << "TestEncodeScriptAnswer passed." << std::endl;
}

void TestEncodeOnClick() {
    uint32_t expectedVid = 999;
    EterBase::EntityId targetId(expectedVid);
    
    auto result = QuestCommandEncoder::EncodeOnClick(targetId);
    assert(result.has_value());

    const auto& buffer = result.value();
    assert(buffer.size() == sizeof(TPacketCGOnClick));

    TPacketCGOnClick decodedPacket;
    std::memcpy(&decodedPacket, buffer.data(), sizeof(decodedPacket));

    assert(decodedPacket.header == CG::ON_CLICK);
    assert(decodedPacket.length == sizeof(TPacketCGOnClick));
    assert(decodedPacket.vid == expectedVid);

    std::cout << "TestEncodeOnClick passed." << std::endl;
}

int main() {
    TestEncodeScriptAnswer();
    TestEncodeOnClick();
    
    std::cout << "All QuestCommandEncoder tests passed." << std::endl;
    return 0;
}
