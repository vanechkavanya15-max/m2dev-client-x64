#include "ChatCommandHandler.h"
#include <algorithm>
#include <cctype>
#include <expected>

namespace Client::Gameplay {

EterBase::Result<void, CommandError> ChatCommandHandler::HandleMessage(const ChatMessage& message) {
    if (auto validationResult = ValidateMessage(message.text); !validationResult) {
        return std::unexpected(validationResult.error());
    }

    switch (message.type) {
        case ChatType::Normal:
            return RouteNormal(message);
        case ChatType::Shout:
            return RouteShout(message);
        case ChatType::Group:
            return RouteGroup(message);
        case ChatType::Guild:
            return RouteGuild(message);
        default:
            return std::unexpected(CommandError::InvalidChatType);
    }
}

EterBase::Result<void, CommandError> ChatCommandHandler::ValidateMessage(const std::string& text) const {
    if (text.empty()) {
        return std::unexpected(CommandError::EmptyMessage);
    }

    if (text.length() > 128) {
        return std::unexpected(CommandError::MessageTooLong);
    }

    bool allSpaces = std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); });
    if (allSpaces) {
        return std::unexpected(CommandError::EmptyMessage);
    }

    return {};
}

EterBase::Result<void, CommandError> ChatCommandHandler::RouteNormal(const ChatMessage& message) const {
    // Normal chat processing logic
    return {};
}

EterBase::Result<void, CommandError> ChatCommandHandler::RouteShout(const ChatMessage& message) const {
    if (message.playerLevel < 15) {
        return std::unexpected(CommandError::LevelTooLowForShout);
    }
    // Shout chat processing logic
    return {};
}

EterBase::Result<void, CommandError> ChatCommandHandler::RouteGroup(const ChatMessage& message) const {
    // Group chat processing logic
    return {};
}

EterBase::Result<void, CommandError> ChatCommandHandler::RouteGuild(const ChatMessage& message) const {
    // Guild chat processing logic
    return {};
}

} // namespace Client::Gameplay
