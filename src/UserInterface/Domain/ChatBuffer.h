#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <deque>

namespace Metin2::UserInterface::Domain
{

#pragma pack(push, 1)

/**
 * @brief Represents the raw network packet structure for incoming chat.
 * 
 * This structure maps directly to the protocol definition and must have
 * no implicit padding to accurately parse the binary payload.
 */
struct ChatPacketHeader
{
    uint16_t header;
    uint16_t length;
    uint8_t type;
    uint32_t targetId;
    uint8_t empireId;
};

#pragma pack(pop)

/**
 * @brief Represents a single chat message stored in the history buffer.
 */
struct ChatMessage
{
    uint8_t type;
    uint32_t targetId;
    uint8_t empireId;
    std::string text;
};

/**
 * @brief Manages a bounded log of recent chat messages.
 * 
 * Operates autonomously in memory without direct ties to GUI rendering,
 * enforcing state encapsulation and memory limits.
 */
class ChatBuffer
{
public:
    /**
     * @brief Constructs a ChatBuffer with the specified capacity limit.
     * @param maxCapacity The maximum number of messages to retain in memory (default: 300).
     */
    explicit ChatBuffer(size_t maxCapacity = 300) : capacity(maxCapacity) {}

    /**
     * @brief Default destructor.
     */
    ~ChatBuffer() = default;

    /**
     * @brief Parses a binary payload and appends the extracted message to the buffer.
     * @param buffer A span covering the raw binary packet data.
     * @return true if the packet was successfully decoded and added, false if invalid or incomplete.
     */
    bool AppendFromPacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(ChatPacketHeader))
        {
            return false;
        }

        const auto* packetHeader = reinterpret_cast<const ChatPacketHeader*>(buffer.data());
        
        if (buffer.size() < packetHeader->length)
        {
            return false;
        }

        size_t textLength = packetHeader->length - sizeof(ChatPacketHeader);
        std::string_view textPayload(
            reinterpret_cast<const char*>(buffer.data() + sizeof(ChatPacketHeader)),
            textLength
        );

        // Remove null terminator from the end if it exists in the payload.
        if (!textPayload.empty() && textPayload.back() == '\0')
        {
            textPayload.remove_suffix(1);
        }

        AppendMessage(packetHeader->type, packetHeader->targetId, packetHeader->empireId, textPayload);
        return true;
    }

    /**
     * @brief Appends a new chat message to the history.
     * @param type The category or type of the chat message.
     * @param targetId The ID of the speaker or target entity.
     * @param empireId The empire affiliation of the speaker.
     * @param text The actual message content.
     */
    void AppendMessage(uint8_t type, uint32_t targetId, uint8_t empireId, std::string_view text)
    {
        messages.push_back(ChatMessage{type, targetId, empireId, std::string(text)});

        while (messages.size() > capacity)
        {
            messages.pop_front();
        }
    }

    /**
     * @brief Clears all messages currently held in the buffer.
     */
    void Clear()
    {
        messages.clear();
    }

    /**
     * @brief Retrieves the collection of stored messages.
     * @return A constant reference to the internal deque of chat messages.
     */
    [[nodiscard]] const std::deque<ChatMessage>& GetMessages() const noexcept
    {
        return messages;
    }

    /**
     * @brief Gets the current maximum capacity of the buffer.
     * @return The maximum number of messages allowed before old ones are truncated.
     */
    [[nodiscard]] size_t GetCapacity() const noexcept
    {
        return capacity;
    }

    /**
     * @brief Modifies the capacity of the buffer, truncating oldest messages if needed.
     * @param newCapacity The new maximum size of the buffer.
     */
    void SetCapacity(size_t newCapacity)
    {
        capacity = newCapacity;
        while (messages.size() > capacity)
        {
            messages.pop_front();
        }
    }

private:
    size_t capacity;
    std::deque<ChatMessage> messages;
};

} // namespace Metin2::UserInterface::Domain
