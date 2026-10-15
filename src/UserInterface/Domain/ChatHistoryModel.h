#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <expected>
#include <span>
#include <algorithm>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Packets/Packet_Chat.h"

namespace UserInterface::Domain {

/**
 * @brief Event triggered when a new chat message is added to the history.
 */
struct ChatMessageAddedEvent : public Core::IEvent {
    Network::Packets::ChatType type;
    std::string message;

    /**
     * @brief Constructs the chat message added event.
     * @param type The type of the chat message.
     * @param message The content of the chat message.
     */
    ChatMessageAddedEvent(Network::Packets::ChatType type, std::string message)
        : type(type), message(std::move(message)) {}
};

/**
 * @brief Represents a single chat message in the history.
 */
struct ChatMessage {
    Network::Packets::ChatType type;
    std::string text;
    // std::optional<EterBase::EntityId> senderId; // If we wanted to store sender

    /**
     * @brief Constructs a new ChatMessage.
     * @param type The type of the chat message.
     * @param text The content of the chat message.
     */
    ChatMessage(Network::Packets::ChatType type, std::string text)
        : type(type), text(std::move(text)) {}
};

/**
 * @brief Domain model managing chat history as a ring buffer with channel filtering.
 * 
 * Replaces the old CPythonChat system with a decoupled, C++23 event-driven architecture.
 * Memory leaks from manual pool allocations are eliminated by using modern containers.
 */
class ChatHistoryModel {
public:
    static constexpr size_t DEFAULT_MAX_MESSAGES = 300;

    /**
     * @brief Constructs the ChatHistoryModel with a specified maximum capacity.
     * @param maxMessages The maximum number of messages to keep in history.
     */
    explicit ChatHistoryModel(size_t maxMessages = DEFAULT_MAX_MESSAGES)
        : maxMessages_(maxMessages) {}

    /**
     * @brief Adds a new message to the chat history and publishes an event.
     * @param type The type of the chat message.
     * @param message The content of the chat message.
     * @return PacketResult indicating success.
     */
    EterBase::PacketResult<void> AddMessage(Network::Packets::ChatType type, std::string_view message) {
        if (message.empty()) {
            EterBase::ModernLogger::Warn("Attempted to add empty message to ChatHistoryModel");
            return {};
        }

        messages_.emplace_back(type, std::string(message));

        if (messages_.size() > maxMessages_) {
            messages_.pop_front();
        }

        EterBase::ModernLogger::Debug("Added chat message of type {}", static_cast<uint32_t>(type));

        // Emit event to update GUI (use GetInstance as defined in EventBus.h)
        Core::EventBus::GetInstance().Publish(ChatMessageAddedEvent{type, std::string(message)});

        return {};
    }

    /**
     * @brief Retrieves the latest messages up to a given limit.
     * @param count The maximum number of messages to retrieve.
     * @return A vector of messages.
     */
    [[nodiscard]] std::vector<ChatMessage> GetRecentMessages(size_t count) const {
        std::vector<ChatMessage> result;
        size_t startIdx = messages_.size() > count ? messages_.size() - count : 0;
        
        for (size_t i = startIdx; i < messages_.size(); ++i) {
            result.push_back(messages_[i]);
        }
        
        return result;
    }

    /**
     * @brief Filters the chat history by a specific chat type.
     * @param type The chat type to filter by.
     * @return A vector of messages matching the type.
     */
    [[nodiscard]] std::vector<ChatMessage> GetMessagesByType(Network::Packets::ChatType type) const {
        std::vector<ChatMessage> result;
        
        for (const auto& msg : messages_) {
            if (msg.type == type) {
                result.push_back(msg);
            }
        }
        
        return result;
    }

    /**
     * @brief Clears the entire chat history.
     */
    void Clear() {
        messages_.clear();
        EterBase::ModernLogger::Info("Chat history cleared.");
    }

    /**
     * @brief Returns the current number of messages in the history.
     * @return The size of the message buffer.
     */
    [[nodiscard]] size_t GetSize() const {
        return messages_.size();
    }

private:
    size_t maxMessages_;
    std::deque<ChatMessage> messages_;
};

} // namespace UserInterface::Domain
