#include <iostream>
#include <vector>
#include <cstring>
#include "EterBase/Result.h"
#include "Client/Network/Handlers/SafeboxHandler.h"
#include "UserInterface/Packet.h"

int g_testsPassed = 0;
int g_testsFailed = 0;

void AssertEqual(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "[PASS] " << testName << std::endl;
        g_testsPassed++;
    } else {
        std::cout << "[FAIL] " << testName << std::endl;
        g_testsFailed++;
    }
}

void TestBufferUnderflow() {
    std::vector<uint8_t> shortBuffer(1);
    Client::Gameplay::SafeBox safeBox(135);

    auto result1 = Client::Network::Handlers::HandleSafeboxSet(shortBuffer, safeBox);
    AssertEqual(!result1.has_value() && result1.error() == EterBase::PacketError::BufferUnderflow, "HandleSafeboxSet Underflow");

    auto result2 = Client::Network::Handlers::HandleSafeboxDel(shortBuffer, safeBox);
    AssertEqual(!result2.has_value() && result2.error() == EterBase::PacketError::BufferUnderflow, "HandleSafeboxDel Underflow");

    auto result3 = Client::Network::Handlers::HandleSafeboxSize(shortBuffer);
    AssertEqual(!result3.has_value() && result3.error() == EterBase::PacketError::BufferUnderflow, "HandleSafeboxSize Underflow");

    auto result4 = Client::Network::Handlers::HandleSafeboxWrongPassword(shortBuffer);
    AssertEqual(!result4.has_value() && result4.error() == EterBase::PacketError::BufferUnderflow, "HandleSafeboxWrongPassword Underflow");
}

void TestHandleSafeboxSet() {
    Client::Gameplay::SafeBox safeBox(135);
    TPacketGCItemSet packet{};
    packet.header = 0x0830;
    packet.pos.cell = 5;
    packet.vnum = 12345;
    packet.count = 2;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));

    auto result = Client::Network::Handlers::HandleSafeboxSet(buffer, safeBox);
    AssertEqual(result.has_value(), "HandleSafeboxSet Returns Success");

    auto itemOpt = safeBox.GetItem(EterBase::ItemSlot{5});
    AssertEqual(itemOpt.has_value() && itemOpt->vnum.get() == 12345 && itemOpt->count == 2, "HandleSafeboxSet Domain State Update");
}

void TestHandleSafeboxDel() {
    Client::Gameplay::SafeBox safeBox(135);
    
    // Setup domain state directly
    safeBox.SetItem(EterBase::ItemSlot{10}, Client::Gameplay::SafeBoxItem{EterBase::ItemVnum{999}, 1});
    AssertEqual(safeBox.GetItem(EterBase::ItemSlot{10}).has_value(), "HandleSafeboxDel Pre-Condition Setup");

    TPacketGCItemDel packet{};
    packet.header = 0x0831;
    packet.pos.cell = 10;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));

    auto result = Client::Network::Handlers::HandleSafeboxDel(buffer, safeBox);
    AssertEqual(result.has_value(), "HandleSafeboxDel Returns Success");

    auto itemOpt = safeBox.GetItem(EterBase::ItemSlot{10});
    AssertEqual(!itemOpt.has_value(), "HandleSafeboxDel Domain State Emptying");
}

void TestHandleSafeboxSize() {
    TPacketGCSafeboxSize packet{};
    packet.header = 0x0833;
    packet.bSize = 3;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));

    auto result = Client::Network::Handlers::HandleSafeboxSize(buffer);
    AssertEqual(result.has_value(), "HandleSafeboxSize Returns Success");
}

void TestHandleSafeboxWrongPassword() {
    TPacketGCSafeboxWrongPassword packet{};
    packet.header = 0x0832;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));

    auto result = Client::Network::Handlers::HandleSafeboxWrongPassword(buffer);
    AssertEqual(result.has_value(), "HandleSafeboxWrongPassword Returns Success");
}

int main() {
    std::cout << "Running SafeBox Handler Tests..." << std::endl;

    TestBufferUnderflow();
    TestHandleSafeboxSet();
    TestHandleSafeboxDel();
    TestHandleSafeboxSize();
    TestHandleSafeboxWrongPassword();

    std::cout << "\nTest Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed." << std::endl;

    return g_testsFailed == 0 ? 0 : 1;
}
