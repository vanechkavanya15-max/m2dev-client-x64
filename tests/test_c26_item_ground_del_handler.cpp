#include <vector>
#include <iostream>
#include <cassert>
#include <cstring>
#include <expected>
#include <cstdint>

// MOCK: CPythonItem
class CPythonItem {
public:
    static CPythonItem& Instance() { static CPythonItem instance; return instance; }
    void DeleteItem(uint32_t vid) { delete_count++; last_deleted_vid = vid; }
    int delete_count = 0;
    uint32_t last_deleted_vid = 0;
};
#define StdAfx_h

// Instead of including the real PythonItem, we mocked it above.
// Now we define a macro so the #include inside the cpp will find nothing.
#define _USERINTERFACE_PYTHONITEM_H_

#include "../src/Client/Network/Handlers/ItemGroundDelHandler.h"

// Tracker for EventBus
struct TestTracker {
    int publish_count = 0;
};
TestTracker g_Tracker;

void test_buffer_underflow() {
    std::vector<uint8_t> buffer = { 0x01, 0x02, 0x03 }; 
    auto result = ItemGroundDelHandler::Handle(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
    std::cout << "[PASS] test_buffer_underflow\n";
}

void test_successful_parsing() {
    CPythonItem::Instance().delete_count = 0;
    g_Tracker.publish_count = 0;

    // Subscribe to EventBus
    UserInterface::Core::EventBus::GetInstance().Subscribe<ItemGroundDelEvent>([](const ItemGroundDelEvent& e) {
        g_Tracker.publish_count++;
    });

    ItemGroundDelPacket pkt;
    pkt.header = 0x55;
    pkt.itemVid = 12345;
    
    std::vector<uint8_t> buffer(sizeof(ItemGroundDelPacket));
    std::memcpy(buffer.data(), &pkt, sizeof(ItemGroundDelPacket));
    
    auto result = ItemGroundDelHandler::Handle(buffer);
    assert(result.has_value());
    assert(CPythonItem::Instance().delete_count == 1);
    assert(CPythonItem::Instance().last_deleted_vid == 12345);
    assert(g_Tracker.publish_count == 1);
    
    std::cout << "[PASS] test_successful_parsing\n";
}

int main() {
    std::cout << "Running tests for ItemGroundDelHandler...\n";
    test_buffer_underflow();
    test_successful_parsing();
    std::cout << "All tests passed.\n";
    return 0;
}
