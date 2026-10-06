#pragma once

#include <string_view>
#include <span>
#include <functional>
#include <cstdint>
#include <string>

#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Core {

/**
 * @brief Event triggered when a quest input string packet is successfully sent to the server.
 * 
 * Replaces direct Python UI calls to achieve decoupling. Subsystems can listen to this event.
 */
struct QuestInputSentEvent : public IEvent {
    std::string input;

    /**
     * @brief Constructs the quest input sent event.
     * @param input The text that was sent to the server.
     */
    explicit QuestInputSentEvent(std::string input) : input(std::move(input)) {}
};

} // namespace UserInterface::Core

namespace Network {

/**
 * @brief Formats and sends a quest input string packet to the server.
 * 
 * @param inputText The string text input by the user for the quest prompt. Must not exceed 64 characters.
 * @param sendCallback A callback function taking a span of bytes to send over the network.
 * @return EterBase::PacketResult<void> Returns success or a specific packet error.
 */
EterBase::PacketResult<void> SendQuestInputPacket(
    std::string_view inputText,
    const std::function<bool(std::span<const uint8_t>)>& sendCallback);

} // namespace Network
