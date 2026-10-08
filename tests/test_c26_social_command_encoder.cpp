#include "../src/Client/Network/SocialCommandEncoder.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <span>

using namespace Client::Network;

void TestEncodeChat() {
    ChatCommand cmd;
    cmd.type = 1; // e.g., CHAT_TYPE_TALKING
    cmd.message = "Hello, world!";

    auto result = SocialCommandEncoder::EncodeChat(cmd);
    assert(result.has_value());

    const auto& buffer = result.value();
    size_t expectedSize = sizeof(TPacketCGChat) + cmd.message.length();
    assert(buffer.size() == expectedSize);

    TPacketCGChat header;
    std::memcpy(&header, buffer.data(), sizeof(TPacketCGChat));
    assert(header.header == CG::CHAT);
    assert(header.length == expectedSize);
    assert(header.type == cmd.type);

    std::string decodedMsg(reinterpret_cast<const char*>(buffer.data() + sizeof(TPacketCGChat)), cmd.message.length());
    assert(decodedMsg == cmd.message);

    std::cout << "[OK] TestEncodeChat passed." << std::endl;
}

void TestEncodeWhisper() {
    WhisperCommand cmd;
    cmd.targetName = "TargetUser123";
    cmd.message = "This is a whisper message.";

    auto result = SocialCommandEncoder::EncodeWhisper(cmd);
    assert(result.has_value());

    const auto& buffer = result.value();
    size_t expectedSize = sizeof(TPacketCGWhisper) + cmd.message.length();
    assert(buffer.size() == expectedSize);

    TPacketCGWhisper header;
    std::memcpy(&header, buffer.data(), sizeof(TPacketCGWhisper));
    assert(header.header == CG::WHISPER);
    assert(header.length == expectedSize);
    
    std::string decodedName(header.szNameTo, strnlen(header.szNameTo, CHARACTER_NAME_MAX_LEN));
    assert(decodedName == cmd.targetName);

    std::string decodedMsg(reinterpret_cast<const char*>(buffer.data() + sizeof(TPacketCGWhisper)), cmd.message.length());
    assert(decodedMsg == cmd.message);

    std::cout << "[OK] TestEncodeWhisper passed." << std::endl;
}

void TestEncodeWhisper_NameTooLong() {
    WhisperCommand cmd;
    cmd.targetName = "ThisNameIsWayTooLongAndShouldBeTruncatedToMaximumLengthAllowedByTheSystem";
    cmd.message = "Hello";

    auto result = SocialCommandEncoder::EncodeWhisper(cmd);
    assert(result.has_value());

    const auto& buffer = result.value();
    TPacketCGWhisper header;
    std::memcpy(&header, buffer.data(), sizeof(TPacketCGWhisper));
    
    std::string decodedName(header.szNameTo, strnlen(header.szNameTo, CHARACTER_NAME_MAX_LEN));
    // It should be truncated to CHARACTER_NAME_MAX_LEN
    assert(decodedName.length() == CHARACTER_NAME_MAX_LEN);
    assert(decodedName == cmd.targetName.substr(0, CHARACTER_NAME_MAX_LEN));
    assert(header.szNameTo[CHARACTER_NAME_MAX_LEN] == '\0');

    std::cout << "[OK] TestEncodeWhisper_NameTooLong passed." << std::endl;
}

int main() {
    std::cout << "Running SocialCommandEncoder tests..." << std::endl;
    TestEncodeChat();
    TestEncodeWhisper();
    TestEncodeWhisper_NameTooLong();
    std::cout << "All SocialCommandEncoder tests passed." << std::endl;
    return 0;
}
