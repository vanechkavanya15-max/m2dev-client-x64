#pragma once

#include <cstdint>
#include <string_view>
#include <string>
#include <span>
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{

/**
 * @brief Event triggered when a private whisper message is received.
 * 
 * Used to decouple the network packet handling logic from the GUI layer (e.g., CPythonChat).
 * Subsystems should listen to this event via EventBus to update their state or UI.
 */
struct WhisperMessageReceivedEvent : public UserInterface::Core::IEvent
{
    uint8_t type;
    std::string senderName;
    std::string message;

    /**
     * @brief Constructs the whisper message received event.
     * @param type The type of the whisper (e.g., WHISPER_TYPE_CHAT).
     * @param senderName The name of the player sending the whisper.
     * @param message The content of the private message.
     */
    WhisperMessageReceivedEvent(uint8_t type, std::string senderName, std::string message)
        : type(type), senderName(std::move(senderName)), message(std::move(message)) {}
};

#pragma pack(push, 1)
/**
 * @brief Modern C++23 structural representation of the GC Whisper packet header (TPacketGCWhisper).
 * Ensures memory alignment identically to legacy protocols.
 */
struct PacketWhisperHeader
{
    uint16_t header;
    uint16_t length;
    uint8_t type;
    char senderName[CHARACTER_NAME_MAX_LEN + 1];
};
#pragma pack(pop)

/**
 * @brief Processes the incoming whisper message packet payload.
 * 
 * Safely parses the data using std::span to ensure bounds checking and extracts
 * both the sender's name and message text via std::string_view without allocations.
 * Once parsed, it broadcasts a WhisperMessageReceivedEvent over the EventBus.
 * 
 * @param buffer The binary buffer containing the whisper network packet.
 * @return EterBase::PacketResult<void> indicating success or returning a parsed PacketError.
 */
EterBase::PacketResult<void> ProcessWhisperMessage(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
