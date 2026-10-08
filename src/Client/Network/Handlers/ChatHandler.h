#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../UserInterface/Core/EventBus.h"

namespace Client::Network::Handlers
{

constexpr std::size_t CHARACTER_NAME_MAX_LEN = 64;

#pragma pack(push, 1)
struct PacketChatHeader
{
    uint16_t header;
    uint16_t length;
    uint8_t type;
    uint32_t dwVID;
    uint8_t bEmpire;
};

struct PacketWhisperHeader
{
    uint16_t header;
    uint16_t length;
    uint8_t type;
    char szNameFrom[CHARACTER_NAME_MAX_LEN + 1];
};
#pragma pack(pop)

struct ChatMessageReceivedEvent : public UserInterface::Core::IEvent
{
    uint8_t type;
    uint32_t dwVID;
    uint8_t bEmpire;
    std::string message;

    ChatMessageReceivedEvent(uint8_t type, uint32_t dwVID, uint8_t bEmpire, std::string message)
        : type(type), dwVID(dwVID), bEmpire(bEmpire), message(std::move(message)) {}
};

struct WhisperMessageReceivedEvent : public UserInterface::Core::IEvent
{
    uint8_t type;
    std::string senderName;
    std::string message;

    WhisperMessageReceivedEvent(uint8_t type, std::string senderName, std::string message)
        : type(type), senderName(std::move(senderName)), message(std::move(message)) {}
};

[[nodiscard]] bool IsValidUtf8(std::string_view text);

EterBase::PacketResult<void> ProcessChatMessage(std::span<const uint8_t> buffer);
EterBase::PacketResult<void> ProcessWhisperMessage(std::span<const uint8_t> buffer);

} // namespace Client::Network::Handlers
