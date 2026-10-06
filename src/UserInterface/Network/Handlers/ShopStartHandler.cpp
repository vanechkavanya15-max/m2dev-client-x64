#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonShop.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include <optional>
#include <span>
#include <cstring>

/**
 * @file ShopStartHandler.cpp
 * @brief Handles network packets for opening NPC and private shops.
 */

using namespace EterBase;

namespace UserInterface::Core {
    /**
     * @brief Event triggered to request opening the shop UI.
     * Defined locally here to respect the strict zero-conflict policy.
     */
    struct ShopOpenEvent : public IEvent {
        uint32_t ownerId;

        /**
         * @brief Constructs the shop open event.
         * @param ownerId The unique identifier of the shop owner.
         */
        explicit ShopOpenEvent(uint32_t ownerId) : ownerId(ownerId) {}
    };
}

/**
 * @brief Parses a standard shop start packet and updates the local shop memory.
 * 
 * @param payload The binary data containing the owner VID and items list.
 * @return PacketResult<void> Result indicating success or a specific packet error.
 */
PacketResult<void> HandleShopStart(std::span<const uint8_t> payload)
{
    // The payload for SHOP_START includes the target ID (uint32_t) followed by the TPacketGCShopStart structure.
    constexpr size_t minSize = sizeof(uint32_t) + sizeof(TPacketGCShopStart);
    if (payload.size() < minSize)
    {
        ModernLogger::Error("BufferUnderflow in HandleShopStart. Expected {}, got {}", minSize, payload.size());
        return MakeError(PacketError::BufferUnderflow);
    }

    uint32_t rawOwnerId = 0;
    std::memcpy(&rawOwnerId, payload.data(), sizeof(uint32_t));
    const EntityId ownerId(rawOwnerId);
    
    // Using memcpy to safely extract the packet structure without breaking strict aliasing/alignment rules.
    TPacketGCShopStart shopStartPacket;
    std::memcpy(&shopStartPacket, payload.data() + sizeof(uint32_t), sizeof(TPacketGCShopStart));
    
    CPythonShop::Instance().Clear();
    
    for (uint8_t itemIndex = 0; itemIndex < SHOP_HOST_ITEM_MAX_NUM; ++itemIndex)
    {
        TShopItemData itemData;
        std::memcpy(&itemData, &shopStartPacket.items[itemIndex], sizeof(TShopItemData));
        CPythonShop::Instance().SetItemData(itemIndex, itemData);
    }
    
    // Publish event for UI update
    UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::ShopOpenEvent(ownerId.value()));

    return {};
}

/**
 * @brief Parses an extended shop start packet with multiple tabs and updates the local shop memory.
 * 
 * @param payload The binary data containing the owner VID, tab count, and items for each tab.
 * @return PacketResult<void> Result indicating success or a specific packet error.
 */
PacketResult<void> HandleShopStartEx(std::span<const uint8_t> payload)
{
    if (payload.size() < sizeof(TPacketGCShopStartEx))
    {
        ModernLogger::Error("BufferUnderflow in HandleShopStartEx header. Expected {}, got {}", sizeof(TPacketGCShopStartEx), payload.size());
        return MakeError(PacketError::BufferUnderflow);
    }

    TPacketGCShopStartEx shopStartPacket;
    std::memcpy(&shopStartPacket, payload.data(), sizeof(TPacketGCShopStartEx));
    size_t readPoint = sizeof(TPacketGCShopStartEx);
    
    const EntityId ownerId(shopStartPacket.owner_vid);
    const uint8_t tabCount = shopStartPacket.shop_tab_count;
    
    // Check if we have enough data for all tabs
    const size_t requiredSize = readPoint + (tabCount * sizeof(TPacketGCShopStartEx::TSubPacketShopTab));
    if (payload.size() < requiredSize)
    {
        ModernLogger::Error("BufferUnderflow in HandleShopStartEx tabs. Expected {}, got {}", requiredSize, payload.size());
        return MakeError(PacketError::BufferUnderflow);
    }

    CPythonShop::Instance().Clear();
    CPythonShop::Instance().SetTabCount(tabCount);

    for (uint8_t i = 0; i < tabCount; ++i)
    {
        TPacketGCShopStartEx::TSubPacketShopTab packTab;
        std::memcpy(&packTab, payload.data() + readPoint, sizeof(TPacketGCShopStartEx::TSubPacketShopTab));
        readPoint += sizeof(TPacketGCShopStartEx::TSubPacketShopTab);

        CPythonShop::Instance().SetTabCoinType(i, packTab.coin_type);
        
        // Ensure string safety and null termination
        std::string tabName(packTab.name, strnlen(packTab.name, sizeof(packTab.name)));
        CPythonShop::Instance().SetTabName(i, tabName.c_str());

        for (uint8_t j = 0; j < SHOP_HOST_ITEM_MAX_NUM; ++j)
        {
            TShopItemData itemData;
            std::memcpy(&itemData, &packTab.items[j], sizeof(TShopItemData));
            CPythonShop::Instance().SetItemData(i, j, itemData);
        }
    }

    // Publish event for UI update
    UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::ShopOpenEvent(ownerId.value()));

    return {};
}

