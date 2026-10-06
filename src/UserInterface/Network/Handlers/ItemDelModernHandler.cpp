/**
 * @file ItemDelModernHandler.cpp
 * @brief Modern C++23 network handler for processing item deletion from inventory.
 */

#include "EterBase/StrongTypes.h"
#include "EterBase/PacketResult.h"
#include "EterBase/ModernLogger.h"
#include "Core/EventBus.h"
#include "UserInterface/PythonPlayer.h"

#include <expected>
#include <optional>
#include <format>
#include <cstdint>
#include <string>
#include <string_view>

#pragma pack(push, 1)
/**
 * @brief Packet sent from server to client to delete an item from inventory.
 */
struct TPacketGCItemDel {
    uint16_t header;
    uint16_t length;
    uint8_t windowType;
    ItemSlot cell;
};
#pragma pack(pop)

/**
 * @brief Event published when an item is successfully deleted.
 */
struct ItemDeletedEvent {
    uint8_t windowType;
    ItemSlot cell;
};

/**
 * @class ItemDelModernHandler
 * @brief Handles the TPacketGCItemDel packet to remove an item from a specific slot in the inventory.
 */
class ItemDelModernHandler {
public:
    /**
     * @brief Processes the TPacketGCItemDel packet.
     * @param packet The packet received from the server.
     * @return EterBase::PacketResult<void> Returns empty success, or an error string if validation fails.
     */
    EterBase::PacketResult<void> Process(const TPacketGCItemDel& packet) {
        // Example of monads for validation (replacing deep if trees).
        // Let's assume cell 0xFFFF is an invalid cell for deletion.
        auto validatedPacket = std::make_optional(packet)
            .and_then([](const TPacketGCItemDel& p) -> std::optional<TPacketGCItemDel> {
                if (static_cast<uint16_t>(p.cell) == 0xFFFF) {
                    return std::nullopt;
                }
                return p;
            });

        if (!validatedPacket) {
            auto errorMsg = std::format("Invalid cell index {} for item deletion.", static_cast<uint16_t>(packet.cell));
            EterBase::ModernLogger::Error(errorMsg);
            return std::unexpected(errorMsg);
        }

        const auto& validPacket = *validatedPacket;

        // Clear the item data in the local player state (decoupled from GUI).
        CPythonPlayer::Instance().ClearItemData(validPacket.windowType, validPacket.cell);

        // Publish event for GUI and other systems to react without direct coupling.
        Core::EventBus::Instance().Publish(ItemDeletedEvent{
            .windowType = validPacket.windowType,
            .cell = validPacket.cell
        });

        EterBase::ModernLogger::Info(std::format("Successfully processed item deletion at window {}, cell {}", static_cast<unsigned int>(validPacket.windowType), static_cast<uint16_t>(validPacket.cell)));
        return {};
    }
};
