#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <cstring>

#pragma pack(push, 1)

#define QUEST_INPUT_STRING_MAX_NUM 64

namespace CG {
    constexpr uint16_t SCRIPT_ANSWER      = 0x0901;
    constexpr uint16_t QUEST_CONFIRM      = 0x0905;
}

namespace GC {
    constexpr uint16_t SCRIPT             = 0x0910;
    constexpr uint16_t QUEST_CONFIRM      = 0x0911;
    constexpr uint16_t QUEST_INFO         = 0x0912;
}

typedef struct command_script_answer {
    uint16_t    header;
    uint16_t    length;
    uint8_t     answer;
} TPacketCGScriptAnswer;

typedef struct command_quest_confirm {
    uint16_t    header;
    uint16_t    length;
    uint8_t     answer;
    uint32_t    requestPID;
} TPacketCGQuestConfirm;

typedef struct packet_quest_info {
    uint16_t    header;
    uint16_t    length;
    uint16_t    index;
    uint8_t     flag;
} TPacketGCQuestInfo;

typedef struct packet_quest_confirm {
    uint16_t    header;
    uint16_t    length;
    char        msg[64+1];
    int32_t     timeout;
    uint32_t    requestPID;
} TPacketGCQuestConfirm;

typedef struct packet_header {
    uint16_t    header;
} TPacketHeader;

typedef struct command_dynamic_size {
    uint16_t    header;
    uint16_t    length;
} TDynamicSizePacketHeader;

#pragma pack(pop)

#define PACKET_MOCK_H

#include "../src/Client/Network/QuestPacketCodec.h"
#include "../src/Client/Network/QuestPacketCodec.cpp"

using namespace Client::Network;

void test_DecodeQuestInfo_BufferUnderflow() {
    std::vector<uint8_t> buffer(sizeof(TPacketGCQuestInfo) - 1, 0);
    auto result = QuestPacketCodec::DecodeQuestInfo(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
}

void test_DecodeQuestInfo_Success() {
    std::vector<uint8_t> buffer(sizeof(TPacketGCQuestInfo), 0);
    TPacketGCQuestInfo* packet = reinterpret_cast<TPacketGCQuestInfo*>(buffer.data());
    packet->header = GC::QUEST_INFO;
    packet->length = sizeof(TPacketGCQuestInfo);
    packet->index = 42;
    packet->flag = 1;

    auto result = QuestPacketCodec::DecodeQuestInfo(buffer);
    assert(result.has_value());
    assert(result.value().header == GC::QUEST_INFO);
    assert(result.value().index == 42);
    assert(result.value().flag == 1);
}

void test_DecodeQuestConfirm_BufferUnderflow() {
    std::vector<uint8_t> buffer(sizeof(TPacketGCQuestConfirm) - 1, 0);
    auto result = QuestPacketCodec::DecodeQuestConfirm(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
}

void test_DecodeQuestConfirm_Success() {
    std::vector<uint8_t> buffer(sizeof(TPacketGCQuestConfirm), 0);
    TPacketGCQuestConfirm* packet = reinterpret_cast<TPacketGCQuestConfirm*>(buffer.data());
    packet->header = GC::QUEST_CONFIRM;
    packet->length = sizeof(TPacketGCQuestConfirm);
    packet->timeout = 1000;
    packet->requestPID = 12345;
    std::strncpy(packet->msg, "Hello Quest", sizeof(packet->msg));

    auto result = QuestPacketCodec::DecodeQuestConfirm(buffer);
    assert(result.has_value());
    assert(result.value().header == GC::QUEST_CONFIRM);
    assert(result.value().timeout == 1000);
    assert(result.value().requestPID == 12345);
    assert(std::string(result.value().msg) == "Hello Quest");
}

void test_DecodeScript_BufferUnderflow_Header() {
    std::vector<uint8_t> buffer(sizeof(TDynamicSizePacketHeader) - 1, 0);
    auto result = QuestPacketCodec::DecodeScript(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
}

void test_DecodeScript_BufferUnderflow_Data() {
    std::vector<uint8_t> buffer(sizeof(TDynamicSizePacketHeader) + 5, 0);
    TDynamicSizePacketHeader* header = reinterpret_cast<TDynamicSizePacketHeader*>(buffer.data());
    header->header = GC::SCRIPT;
    header->length = sizeof(TDynamicSizePacketHeader) + 10; // claims more data than buffer size

    auto result = QuestPacketCodec::DecodeScript(buffer);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
}

void test_DecodeScript_Success() {
    std::string scriptText = "Hello Script\0";
    std::vector<uint8_t> buffer(sizeof(TDynamicSizePacketHeader) + scriptText.size(), 0);
    
    TDynamicSizePacketHeader* header = reinterpret_cast<TDynamicSizePacketHeader*>(buffer.data());
    header->header = GC::SCRIPT;
    header->length = buffer.size();

    std::memcpy(buffer.data() + sizeof(TDynamicSizePacketHeader), scriptText.data(), scriptText.size());

    auto result = QuestPacketCodec::DecodeScript(buffer);
    assert(result.has_value());
    assert(result.value() == "Hello Script");
}

void test_EncodeScriptAnswer() {
    auto buffer = QuestPacketCodec::EncodeScriptAnswer(5);
    assert(buffer.size() == sizeof(TPacketCGScriptAnswer));

    const TPacketCGScriptAnswer* packet = reinterpret_cast<const TPacketCGScriptAnswer*>(buffer.data());
    assert(packet->header == CG::SCRIPT_ANSWER);
    assert(packet->length == sizeof(TPacketCGScriptAnswer));
    assert(packet->answer == 5);
}

void test_EncodeQuestConfirmAnswer() {
    auto buffer = QuestPacketCodec::EncodeQuestConfirmAnswer(1, 9999);
    assert(buffer.size() == sizeof(TPacketCGQuestConfirm));

    const TPacketCGQuestConfirm* packet = reinterpret_cast<const TPacketCGQuestConfirm*>(buffer.data());
    assert(packet->header == CG::QUEST_CONFIRM);
    assert(packet->length == sizeof(TPacketCGQuestConfirm));
    assert(packet->answer == 1);
    assert(packet->requestPID == 9999);
}

int main() {
    test_DecodeQuestInfo_BufferUnderflow();
    test_DecodeQuestInfo_Success();
    test_DecodeQuestConfirm_BufferUnderflow();
    test_DecodeQuestConfirm_Success();
    test_DecodeScript_BufferUnderflow_Header();
    test_DecodeScript_BufferUnderflow_Data();
    test_DecodeScript_Success();
    test_EncodeScriptAnswer();
    test_EncodeQuestConfirmAnswer();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
