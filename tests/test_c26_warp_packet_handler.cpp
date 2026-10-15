#include "../src/Client/Network/Handlers/WarpPacketHandler.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace Client::Network::Handlers;

int main() {
    std::cout << "Running test_c26_warp_packet_handler...\n";

    TPacketGCWarp w{};
    w.header = 0x0501;
    w.length = sizeof(w);
    w.lX = 1234500;
    w.lY = 6789000;
    w.lAddr = 0x7F000001;
    w.wPort = 13000;

    std::vector<uint8_t> buffer(sizeof(w));
    std::memcpy(buffer.data(), &w, sizeof(w));

    auto res = WarpPacketHandler::HandleWarpPacket(buffer);
    assert(res.has_value());

    // Test buffer underflow
    std::vector<uint8_t> shortBuf(sizeof(w) - 1);
    auto underflowRes = WarpPacketHandler::HandleWarpPacket(shortBuf);
    assert(!underflowRes.has_value());

    // Test zero port
    w.wPort = 0;
    std::memcpy(buffer.data(), &w, sizeof(w));
    auto zeroPortRes = WarpPacketHandler::HandleWarpPacket(buffer);
    assert(!zeroPortRes.has_value());

    std::cout << "All WarpPacketHandler tests passed successfully.\n";
    return 0;
}
