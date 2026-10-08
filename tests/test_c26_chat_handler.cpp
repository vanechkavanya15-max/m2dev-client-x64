#include "../src/Client/Network/Handlers/ChatHandler.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>

using namespace Client::Network::Handlers;

static bool chatMessageReceived = false;
static std::string receivedChatMessage;

static bool whisperMessageReceived = false;
static std::string receivedWhisperMessage;
static std::string receivedWhisperSender;

void SetupEventBus() {
    UserInterface::Core::EventBus::GetInstance().Subscribe<ChatMessageReceivedEvent>([](const ChatMessageReceivedEvent& e) {
        chatMessageReceived = true;
        receivedChatMessage = e.message;
    });

    UserInterface::Core::EventBus::GetInstance().Subscribe<WhisperMessageReceivedEvent>([](const WhisperMessageReceivedEvent& e) {
        whisperMessageReceived = true;
        receivedWhisperMessage = e.message;
        receivedWhisperSender = e.senderName;
    });
}

void ResetTestState() {
    chatMessageReceived = false;
    receivedChatMessage.clear();
    whisperMessageReceived = false;
    receivedWhisperMessage.clear();
    receivedWhisperSender.clear();
}

void TestValidUtf8() {
    assert(IsValidUtf8("Hello World!"));
    assert(IsValidUtf8("Zażółć gęślą jaźń")); // Polish
    assert(IsValidUtf8("こんにちは")); // Japanese
    
    // Invalid UTF-8 cases
    const char invalid1[] = {(char)0xC3, (char)0x28, '\0'}; 
    assert(!IsValidUtf8(invalid1));
    
    std::cout << "TestValidUtf8 passed!" << std::endl;
}

void TestProcessChatMessage_Valid() {
    ResetTestState();
    
    std::string text = "Hello Chat";
    std::vector<uint8_t> buffer(sizeof(PacketChatHeader) + text.length() + 1);
    
    PacketChatHeader* header = reinterpret_cast<PacketChatHeader*>(buffer.data());
    header->header = 1;
    header->length = buffer.size();
    header->type = 1;
    header->dwVID = 100;
    header->bEmpire = 1;
    
    std::memcpy(buffer.data() + sizeof(PacketChatHeader), text.c_str(), text.length() + 1);
    
    auto result = ProcessChatMessage(buffer);
    assert(result.has_value());
    assert(chatMessageReceived);
    assert(receivedChatMessage == text);
    
    std::cout << "TestProcessChatMessage_Valid passed!" << std::endl;
}

void TestProcessChatMessage_InvalidUtf8() {
    ResetTestState();
    
    const char invalidText[] = {(char)0xC3, (char)0x28, '\0'}; 
    std::vector<uint8_t> buffer(sizeof(PacketChatHeader) + sizeof(invalidText));
    
    PacketChatHeader* header = reinterpret_cast<PacketChatHeader*>(buffer.data());
    header->header = 1;
    header->length = buffer.size();
    header->type = 1;
    header->dwVID = 100;
    header->bEmpire = 1;
    
    std::memcpy(buffer.data() + sizeof(PacketChatHeader), invalidText, sizeof(invalidText));
    
    auto result = ProcessChatMessage(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::MalformedPayload);
    assert(!chatMessageReceived);
    
    std::cout << "TestProcessChatMessage_InvalidUtf8 passed!" << std::endl;
}

void TestProcessWhisperMessage_Valid() {
    ResetTestState();
    
    std::string text = "Secret Message";
    std::string sender = "Admin";
    std::vector<uint8_t> buffer(sizeof(PacketWhisperHeader) + text.length() + 1);
    
    PacketWhisperHeader* header = reinterpret_cast<PacketWhisperHeader*>(buffer.data());
    header->header = 2;
    header->length = buffer.size();
    header->type = 2;
    std::memset(header->szNameFrom, 0, sizeof(header->szNameFrom));
    std::memcpy(header->szNameFrom, sender.c_str(), sender.length());
    
    std::memcpy(buffer.data() + sizeof(PacketWhisperHeader), text.c_str(), text.length() + 1);
    
    auto result = ProcessWhisperMessage(buffer);
    assert(result.has_value());
    assert(whisperMessageReceived);
    assert(receivedWhisperMessage == text);
    assert(receivedWhisperSender == sender);
    
    std::cout << "TestProcessWhisperMessage_Valid passed!" << std::endl;
}

int main() {
    SetupEventBus();
    
    TestValidUtf8();
    TestProcessChatMessage_Valid();
    TestProcessChatMessage_InvalidUtf8();
    TestProcessWhisperMessage_Valid();
    
    std::cout << "All chat handler tests passed!" << std::endl;
    return 0;
}
