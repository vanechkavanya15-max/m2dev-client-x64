#pragma once

#include <cstdint>
#include <span>
#include <functional>

/**
 * @brief Represents the data structure sent by the server to delete a character from the view.
 * 
 * This packet must be strictly aligned (`#pragma pack(1)`) to ensure compatibility with the
 * network protocol layout.
 */
#pragma pack(push, 1)
struct TPacketGCCharacterDelete
{
    /** @brief The identifier of the packet type (HEADER_GC_CHARACTER_DEL). */
    uint8_t header;

    /** @brief The ID (VID) of the character to delete. */
    uint32_t id;
};
#pragma pack(pop)

/**
 * @brief Handles the character delete packet logic, decoupling it from the UI layer.
 *
 * This function processes the binary buffer from the network, verifies it against the
 * expected packet structure size, and triggers a decoupling notification callback
 * instead of directly querying GUI or Python elements. This conforms to C++20 standard
 * and strict separation of concerns.
 *
 * @param buffer The binary span representing the received packet data.
 * @param onDelete Callback triggered with the character ID when a delete packet is successfully parsed.
 * @return true if the buffer was large enough and parsed successfully, false otherwise.
 */
inline bool HandleCharacterDelete(std::span<const uint8_t> buffer, const std::function<void(uint32_t)>& onDelete)
{
    if (buffer.size() < sizeof(TPacketGCCharacterDelete))
    {
        return false;
    }

    const auto* packet = reinterpret_cast<const TPacketGCCharacterDelete*>(buffer.data());
    
    if (onDelete)
    {
        onDelete(packet->id);
    }
    
    return true;
}
