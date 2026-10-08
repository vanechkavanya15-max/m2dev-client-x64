#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include "EterBase/Result.h"

namespace Client::Gameplay {

enum class CommandError : uint8_t {
    None = 0,
    EmptyMessage,
    MessageTooLong,
    LevelTooLowForShout,
    InvalidChatType
};

[[nodiscard]] constexpr std::string_view ToString(CommandError err) noexcept {
    switch (err) {
        case CommandError::None: return "None";
        case CommandError::EmptyMessage: return "EmptyMessage";
        case CommandError::MessageTooLong: return "MessageTooLong";
        case CommandError::LevelTooLowForShout: return "LevelTooLowForShout";
        case CommandError::InvalidChatType: return "InvalidChatType";
    }
    return "UnknownCommandError";
}

enum class ChatType : uint8_t {
    Normal = 0,
    Shout,
    Group,
    Guild
};

struct ChatMessage {
    ChatType type;
    std::string text;
    uint8_t playerLevel;
};

class ChatCommandHandler {
public:
    ChatCommandHandler() = default;
    ~ChatCommandHandler() = default;

    EterBase::Result<void, CommandError> HandleMessage(const ChatMessage& message);

private:
    EterBase::Result<void, CommandError> ValidateMessage(const std::string& text) const;
    
    EterBase::Result<void, CommandError> RouteNormal(const ChatMessage& message) const;
    EterBase::Result<void, CommandError> RouteShout(const ChatMessage& message) const;
    EterBase::Result<void, CommandError> RouteGroup(const ChatMessage& message) const;
    EterBase::Result<void, CommandError> RouteGuild(const ChatMessage& message) const;
};

} // namespace Client::Gameplay
