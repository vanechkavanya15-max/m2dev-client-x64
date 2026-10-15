#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <cstring>

#include "../src/Client/Network/Handlers/ItemDelHandler.h"
#include "../src/Client/Gameplay/InventoryDomain.h"
#include "../src/UserInterface/Packet.h"

using namespace Client::Network::Handlers;
using namespace Client::Gameplay;

class ItemDelHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Przygotowujemy początkowy stan ekwipunku
        ItemData dummyItem;
        dummyItem.vnum = EterBase::ItemVnum(10);
        dummyItem.count = 1;
        dummyItem.size = {1, 1};
        
        auto res = inventoryDomain.SetItem(INVENTORY, EterBase::ItemSlot(5), dummyItem);
        ASSERT_TRUE(res.has_value());
    }

    InventoryDomain inventoryDomain;
};

TEST_F(ItemDelHandlerTest, HandlePacket_BufferUnderflow) {
    std::vector<uint8_t> smallBuffer(sizeof(TPacketGCItemDel) - 1, 0);

    auto result = ItemDelHandler::HandlePacket(std::span<const uint8_t>(smallBuffer), inventoryDomain);
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EterBase::PacketError::BufferUnderflow);
}

TEST_F(ItemDelHandlerTest, HandlePacket_Success) {
    TPacketGCItemDel packet;
    packet.header = 1;
    packet.length = sizeof(TPacketGCItemDel);
    packet.pos.window_type = INVENTORY;
    packet.pos.cell = 5;

    std::vector<uint8_t> buffer(sizeof(TPacketGCItemDel));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCItemDel));

    auto result = ItemDelHandler::HandlePacket(std::span<const uint8_t>(buffer), inventoryDomain);
    
    EXPECT_TRUE(result.has_value());

    // Sprawdzenie czy element zniknal
    auto itemResult = inventoryDomain.GetItem(INVENTORY, EterBase::ItemSlot(5));
    EXPECT_FALSE(itemResult.has_value());
    EXPECT_EQ(itemResult.error(), EterBase::InventoryError::SlotEmpty);
}

TEST_F(ItemDelHandlerTest, HandlePacket_ItemNotExists) {
    TPacketGCItemDel packet;
    packet.header = 1;
    packet.length = sizeof(TPacketGCItemDel);
    packet.pos.window_type = INVENTORY;
    packet.pos.cell = 20; // Puste miejsce

    std::vector<uint8_t> buffer(sizeof(TPacketGCItemDel));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCItemDel));

    auto result = ItemDelHandler::HandlePacket(std::span<const uint8_t>(buffer), inventoryDomain);
    
    // Zgodnie z nasza implementacja zwroci MalformedPayload jesli usuniecie sie nie powiedzie
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EterBase::PacketError::MalformedPayload);
}

