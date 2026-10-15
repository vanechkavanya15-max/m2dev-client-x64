#pragma once

#include <cstdint>

namespace Core::Events
{
#pragma pack(push, 1)

    /**
     * @brief Represents a request to check an item into the safebox being sent to the server.
     */
    struct SafeboxCheckinSentEvent
    {
        uint8_t safeboxSlot;  ///< The target safebox slot.
        uint16_t inventorySlot; ///< The source inventory slot.
    };

#pragma pack(pop)
} // namespace Core::Events
