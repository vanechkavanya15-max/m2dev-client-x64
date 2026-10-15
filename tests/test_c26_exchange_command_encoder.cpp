#include <gtest/gtest.h>
#include "../src/Client/Network/ExchangeCommandEncoder.h"
#include "../src/UserInterface/Packet.h"

using namespace Client::Network;

TEST(ExchangeCommandEncoderTest, EncodeStart_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    uint32_t targetVid = 98765;
    auto result = encoder.EncodeStart(targetVid);
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::START);
    EXPECT_EQ(packet->arg1, targetVid);
    EXPECT_EQ(packet->arg2, 0);
}

TEST(ExchangeCommandEncoderTest, EncodeItemAdd_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    TItemPos pos{INVENTORY, 42};
    uint8_t displayPos = 3;
    auto result = encoder.EncodeItemAdd(pos, displayPos);
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::ITEM_ADD);
    EXPECT_EQ(packet->arg1, 0);
    EXPECT_EQ(packet->arg2, displayPos);
    EXPECT_EQ(packet->Pos.window_type, INVENTORY);
    EXPECT_EQ(packet->Pos.cell, 42);
}

TEST(ExchangeCommandEncoderTest, EncodeItemDel_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    uint8_t pos = 5;
    auto result = encoder.EncodeItemDel(pos);
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::ITEM_DEL);
    EXPECT_EQ(packet->arg1, 0);
    EXPECT_EQ(packet->arg2, pos);
}

TEST(ExchangeCommandEncoderTest, EncodeElkAdd_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    uint32_t amount = 50000;
    auto result = encoder.EncodeElkAdd(amount);
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::ELK_ADD);
    EXPECT_EQ(packet->arg1, amount);
    EXPECT_EQ(packet->arg2, 0);
}

TEST(ExchangeCommandEncoderTest, EncodeAccept_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    auto result = encoder.EncodeAccept();
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::ACCEPT);
    EXPECT_EQ(packet->arg1, 0);
    EXPECT_EQ(packet->arg2, 0);
}

TEST(ExchangeCommandEncoderTest, EncodeCancel_ReturnsCorrectPacket)
{
    ExchangeCommandEncoder encoder;
    auto result = encoder.EncodeCancel();
    ASSERT_TRUE(result.has_value());
    
    auto& buffer = result.value();
    ASSERT_EQ(buffer.size(), sizeof(TPacketCGExchange));
    
    auto* packet = reinterpret_cast<const TPacketCGExchange*>(buffer.data());
    EXPECT_EQ(packet->header, CG::EXCHANGE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGExchange));
    EXPECT_EQ(packet->subheader, ExchangeSub::CG::CANCEL);
    EXPECT_EQ(packet->arg1, 0);
    EXPECT_EQ(packet->arg2, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
