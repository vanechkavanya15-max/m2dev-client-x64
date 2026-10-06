#pragma once

#include <cstdint>
#include <span>
#include <optional>

/**
 * @brief Shop start event result containing the target ID for UI notification.
 */
struct ShopStartEvent
{
    uint32_t targetId;
};

/**
 * @brief Handles incoming network packets related to the shop system.
 * 
 * This class is responsible for parsing raw binary buffers from the network
 * and updating the underlying memory structures (e.g., CPythonShop) without
 * directly invoking Python UI callbacks.
 */
class ShopHandler
{
public:
    /**
     * @brief Parses a standard shop start packet and updates the local shop memory.
     * 
     * @param payload The binary data containing the owner VID and items list.
     * @return std::optional<ShopStartEvent> An event to notify the UI, if parsing was successful.
     */
    static std::optional<ShopStartEvent> HandleShopStart(std::span<const uint8_t> payload);

    /**
     * @brief Parses an extended shop start packet with multiple tabs and updates the local shop memory.
     * 
     * @param payload The binary data containing the owner VID, tab count, and items for each tab.
     * @return std::optional<ShopStartEvent> An event to notify the UI, if parsing was successful.
     */
    static std::optional<ShopStartEvent> HandleShopStartEx(std::span<const uint8_t> payload);
};
