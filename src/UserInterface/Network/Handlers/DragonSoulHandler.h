#pragma once

#include <cstdint>
#include <span>
#include <array>
#include <optional>
#include <vector>

#include "../../Packet.h"
#include "../../GameType.h"

/**
 * @class DragonSoulHandler
 * @brief Handles network communication and internal state for the Dragon Soul alchemy system.
 *
 * This handler operates purely on the C++ state and abstracts away GUI interactions.
 * It tracks the state of the Dragon Soul refine window and processes both incoming (GC)
 * and outgoing (CG) packets related to dragon soul refining.
 */
class DragonSoulHandler
{
public:
    /**
     * @brief Constructs a new Dragon Soul Handler.
     */
    DragonSoulHandler() = default;

    /**
     * @brief Destroys the Dragon Soul Handler.
     */
    ~DragonSoulHandler() = default;

    /**
     * @brief Processes an incoming Dragon Soul refine packet from the server.
     * 
     * Parses the packet to update the internal state based on its sub-header 
     * (e.g. window opened, refine succeeded, refine failed).
     *
     * @param packet_data A span over the bytes containing the TPacketGCDragonSoulRefine packet.
     * @return true if the packet was handled successfully, false if the packet was invalid.
     */
    bool HandleReceiveRefinePacket(std::span<const uint8_t> packet_data);

    /**
     * @brief Adds an item to a specific slot in the Dragon Soul refine window.
     *
     * @param slot_index The index in the refine window (0 to DS_REFINE_WINDOW_MAX_NUM - 1).
     * @param item_pos The actual position of the item in the inventory.
     * @return true if the item was successfully added, false if the slot index is invalid.
     */
    bool AddItemToRefineSlot(uint32_t slot_index, const TItemPos& item_pos);

    /**
     * @brief Clears a specific slot in the Dragon Soul refine window.
     *
     * @param slot_index The index in the refine window to clear.
     */
    void ClearRefineSlot(uint32_t slot_index);

    /**
     * @brief Clears all slots in the Dragon Soul refine window.
     */
    void ClearAllRefineSlots();

    /**
     * @brief Serializes the current refine state into a packet to send to the server.
     *
     * @param refine_type The sub-type of the refine operation (e.g., DO_UPGRADE, DO_IMPROVEMENT).
     * @return A vector of bytes representing the serialized TPacketCGDragonSoulRefine. 
     *         If the refine state is invalid (e.g., no items placed), an empty vector may be returned.
     */
    std::vector<uint8_t> BuildSendRefinePacket(uint8_t refine_type) const;

    /**
     * @brief Checks if the Dragon Soul refine window is currently considered open.
     *
     * @return true if open, false otherwise.
     */
    bool IsWindowOpen() const;

private:
    /// Indicates whether the refine window is active/open.
    bool m_isWindowOpen{false};

    /// Array storing the positions of items currently placed in the refine window slots.
    std::array<std::optional<TItemPos>, DS_REFINE_WINDOW_MAX_NUM> m_refineSlots;
};
