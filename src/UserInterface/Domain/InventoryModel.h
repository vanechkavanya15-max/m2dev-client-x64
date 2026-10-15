#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <functional>
#include <string_view>
#include <string>
#include <span>
#include <stdexcept>

/**
 * @brief Represents a single slot for item attributes (bonus).
 */
struct ItemAttribute
{
    uint16_t type;  ///< Type of the attribute
    int16_t value;  ///< Value of the attribute
};

/**
 * @brief Core data structure representing an item in the inventory.
 */
struct ItemData
{
    uint32_t vnum;                           ///< Item virtual number
    uint32_t count;                          ///< Quantity of the item
    std::string ownerName;                   ///< Optional owner name of the item
    std::vector<uint32_t> sockets;           ///< Socket values (Metin stones)
    std::vector<ItemAttribute> attributes;   ///< Item attributes/bonuses
    bool isLocked;                           ///< Is item locked for trade/exchange
};

/**
 * @brief Network packet structure for inventory item updates.
 * Must be strictly 1-byte aligned for network transmission.
 */
#pragma pack(push, 1)
struct InventoryItemPacket
{
    uint8_t header;         ///< Packet header identifier
    uint16_t slotIndex;     ///< Target slot index
    uint32_t vnum;          ///< Item virtual number
    uint32_t count;         ///< Item count
    uint32_t sockets[3];    ///< Max 3 sockets
    ItemAttribute attributes[7]; ///< Max 7 attributes
};
#pragma pack(pop)

/**
 * @brief Manages the inventory domain logic and memory state.
 * 
 * Decoupled from the GUI layer; uses callbacks to notify observers of changes.
 */
class InventoryModel
{
public:
    /**
     * @brief Type alias for the callback invoked when a slot updates.
     * @param slotIndex The index of the slot that was updated.
     */
    using SlotUpdateCallback = std::function<void(uint16_t slotIndex)>;

    /**
     * @brief Constructs an InventoryModel with a specific capacity.
     * @param capacity The total number of slots in the inventory.
     */
    explicit InventoryModel(uint16_t capacity)
        : slots(capacity), updateCallback(nullptr)
    {
    }

    /**
     * @brief Registers a callback to be invoked when an inventory slot changes.
     * @param callback The function to call on slot update.
     */
    void setUpdateCallback(SlotUpdateCallback callback)
    {
        updateCallback = std::move(callback);
    }

    /**
     * @brief Sets an item at the specified slot index.
     * @param index The slot index.
     * @param item The item data to set.
     * @return true if successful, false if index is out of bounds.
     */
    bool setItem(uint16_t index, const ItemData& item)
    {
        if (index >= slots.size())
        {
            return false;
        }

        slots[index] = item;
        notifySlotUpdate(index);
        return true;
    }

    /**
     * @brief Sets the owner name for a specific item slot.
     * @param index The slot index.
     * @param ownerName The name of the new owner.
     * @return true if successful, false if index is out of bounds or empty.
     */
    bool setItemOwner(uint16_t index, std::string_view ownerName)
    {
        if (index >= slots.size() || !slots[index].has_value())
        {
            return false;
        }

        slots[index]->ownerName = std::string(ownerName);
        notifySlotUpdate(index);
        return true;
    }

    /**
     * @brief Clears the item at the specified slot.
     * @param index The slot index.
     * @return true if successful, false if index is out of bounds.
     */
    bool clearItem(uint16_t index)
    {
        if (index >= slots.size())
        {
            return false;
        }

        slots[index].reset();
        notifySlotUpdate(index);
        return true;
    }

    /**
     * @brief Retrieves the item at the specified slot.
     * @param index The slot index.
     * @return std::optional containing the item data if present and valid index, std::nullopt otherwise.
     */
    std::optional<ItemData> getItem(uint16_t index) const
    {
        if (index >= slots.size() || !slots[index].has_value())
        {
            return std::nullopt;
        }

        return slots[index];
    }

    /**
     * @brief Parses an incoming network packet buffer to update inventory slots.
     * @param buffer A read-only span of bytes representing one or more inventory packets.
     * @return true if successful, false if buffer is malformed.
     */
    bool processNetworkUpdate(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(InventoryItemPacket))
        {
            return false;
        }

        size_t offset = 0;
        while (offset + sizeof(InventoryItemPacket) <= buffer.size())
        {
            const auto* packet = reinterpret_cast<const InventoryItemPacket*>(buffer.data() + offset);
            
            ItemData data;
            data.vnum = packet->vnum;
            data.count = packet->count;
            data.isLocked = false;
            
            for (size_t i = 0; i < 3; ++i)
            {
                data.sockets.push_back(packet->sockets[i]);
            }
            for (size_t i = 0; i < 7; ++i)
            {
                data.attributes.push_back(packet->attributes[i]);
            }

            setItem(packet->slotIndex, data);
            
            offset += sizeof(InventoryItemPacket);
        }

        return true;
    }

    /**
     * @brief Returns the total capacity of the inventory.
     * @return The number of slots.
     */
    uint16_t getCapacity() const
    {
        return static_cast<uint16_t>(slots.size());
    }

private:
    /**
     * @brief Triggers the registered callback if it exists.
     * @param index The slot index that was updated.
     */
    void notifySlotUpdate(uint16_t index)
    {
        if (updateCallback)
        {
            updateCallback(index);
        }
    }

    std::vector<std::optional<ItemData>> slots;  ///< Array of item slots
    SlotUpdateCallback updateCallback;           ///< Callback for UI decoupling
};
