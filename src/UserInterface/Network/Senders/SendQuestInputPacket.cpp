#include "StdAfx.h"
#include "SendQuestInputPacket.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <algorithm>

namespace Network {

namespace {

#pragma pack(push, 1)
/**
 * @brief Represents the binary structure for the HEADER_CG_QUEST_INPUT_STRING packet.
 * 
 * Strictly packed to 1 byte to match network layout expectations (66 bytes total).
 */
struct PacketQuestInputString {
    uint8_t header;
    char text[64 + 1]; // QUEST_INPUT_STRING_MAX_NUM = 64, +1 for null terminator
};
#pragma pack(pop)

constexpr size_t QUEST_INPUT_STRING_MAX_NUM = 64;
constexpr uint8_t HEADER_CG_QUEST_INPUT_STRING = 30;

} // anonymous namespace

EterBase::PacketResult<void> SendQuestInputPacket(
    std::string_view inputText,
    const std::function<bool(std::span<const uint8_t>)>& sendCallback)
{
    if (inputText.length() > QUEST_INPUT_STRING_MAX_NUM) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "[NETWORK] SendQuestInputPacket: input text length {} exceeds maximum allowed length of {}", inputText.length(), QUEST_INPUT_STRING_MAX_NUM);
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    PacketQuestInputString packet;
    std::memset(&packet, 0, sizeof(packet));

    packet.header = HEADER_CG_QUEST_INPUT_STRING;
    
    if (!inputText.empty()) {
        std::copy_n(inputText.begin(), inputText.length(), packet.text);
    }

    std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

    if (!sendCallback(packetSpan)) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "[NETWORK] SendQuestInputPacket: Failed to send packet through callback.");
        return std::unexpected(EterBase::PacketError::SessionClosed);
    }

    UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::QuestInputSentEvent{std::string(inputText)});

    return {};
}

} // namespace Network
