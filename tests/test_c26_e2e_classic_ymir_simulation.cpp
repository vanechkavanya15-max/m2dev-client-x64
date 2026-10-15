#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <span>
#include <string>
#include <rapidjson/document.h>

#include "../src/Client/Network/TeaCryptoProvider.h"
#include "../src/Client/Network/MovementCommandEncoder.h"
#include "../src/Client/Core/DomainCommands.h"
#include "../src/UserInterface/Packet.h"

using namespace Client::Network;
using namespace Client::Core;

TEST(ClassicYmirE2ESimulation, EncryptionAndMovementCommand) {
    // 1. Wczytanie profilu servers/classic_ymir.json.
    std::ifstream ifs("servers/classic_ymir.json");
    ASSERT_TRUE(ifs.is_open()) << "Failed to open servers/classic_ymir.json";
    
    std::string jsonStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    ASSERT_FALSE(doc.HasParseError()) << "JSON parse error";
    ASSERT_TRUE(doc.HasMember("protocol"));
    const auto& protocol = doc["protocol"];
    
    ASSERT_TRUE(protocol.HasMember("crypto"));
    EXPECT_STREQ(protocol["crypto"].GetString(), "TEA");
    
    ASSERT_TRUE(protocol.HasMember("keySize"));
    EXPECT_EQ(protocol["keySize"].GetInt(), 16);
    
    // 2. Aktywacja TeaCryptoProvider z kluczem 16 bajtow.
    TeaCryptoProvider crypto;
    std::vector<uint8_t> key(16, 0xAB); // Dummy key
    
    auto initResult = crypto.Initialize(key);
    ASSERT_TRUE(initResult.has_value()) << "Crypto initialization failed";
    
    // 3. Szyfrowanie i deszyfrowanie ramek 8-bajtowych blokow TEA.
    std::vector<uint8_t> testBlock = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    std::vector<uint8_t> originalBlock = testBlock;
    
    auto encryptResult = crypto.EncryptBlock(testBlock);
    ASSERT_TRUE(encryptResult.has_value()) << "Encryption failed";
    EXPECT_NE(testBlock, originalBlock);
    
    auto decryptResult = crypto.DecryptBlock(testBlock);
    ASSERT_TRUE(decryptResult.has_value()) << "Decryption failed";
    EXPECT_EQ(testBlock, originalBlock);
    
    // 4. Weryfikacja integralnosci komendy ruchu MoveCommand.
    MoveCommand cmd;
    cmd.destination.x = 100.0f;
    cmd.destination.y = 200.0f;
    cmd.rotation = 90.0f;
    cmd.moveType = 1;
    cmd.clientTimestamp = 123456789;
    
    auto encodeResult = MovementCommandEncoder::Encode(cmd);
    ASSERT_TRUE(encodeResult.has_value()) << "Movement command encoding failed";
    
    std::vector<uint8_t> encodedData = encodeResult.value();
    ASSERT_EQ(encodedData.size(), sizeof(TPacketCGMove));
    
    const TPacketCGMove* packet = reinterpret_cast<const TPacketCGMove*>(encodedData.data());
    
    EXPECT_EQ(packet->header, CG::MOVE);
    EXPECT_EQ(packet->length, sizeof(TPacketCGMove));
    EXPECT_EQ(packet->bFunc, 1);
    EXPECT_EQ(packet->bArg, 1);
    EXPECT_EQ(packet->bRot, static_cast<uint8_t>(90.0f / 5.0f));
    EXPECT_EQ(packet->lX, static_cast<int32_t>(100.0f * 100.0f));
    EXPECT_EQ(packet->lY, static_cast<int32_t>(200.0f * 100.0f));
    EXPECT_EQ(packet->dwTime, 123456789);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
