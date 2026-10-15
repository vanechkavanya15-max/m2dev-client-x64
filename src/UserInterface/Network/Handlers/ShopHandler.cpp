#include "StdAfx.h"
#include "ShopHandler.h"
#include "../../Packet.h"
#include "../../PythonShop.h"
#include <cstring>

std::optional<ShopStartEvent> ShopHandler::HandleShopStart(std::span<const uint8_t> payload)
{
    // The packet payload contains:
    // uint32_t targetId (owner VID)
    // TPacketGCShopStart shopStartPacket
    
    constexpr size_t minSize = sizeof(uint32_t) + sizeof(TPacketGCShopStart);
    if (payload.size() < minSize)
    {
        return std::nullopt;
    }

    uint32_t targetId = *reinterpret_cast<const uint32_t*>(payload.data());
    
    const TPacketGCShopStart* shopStartPacket = reinterpret_cast<const TPacketGCShopStart*>(payload.data() + sizeof(uint32_t));
    
    CPythonShop::Instance().Clear();
    
    for (uint8_t itemIndex = 0; itemIndex < SHOP_HOST_ITEM_MAX_NUM; ++itemIndex)
    {
        TShopItemData itemData;
        std::memcpy(&itemData, &shopStartPacket->items[itemIndex], sizeof(TShopItemData));
        CPythonShop::Instance().SetItemData(itemIndex, itemData);
    }
    
    return ShopStartEvent{ targetId };
}

std::optional<ShopStartEvent> ShopHandler::HandleShopStartEx(std::span<const uint8_t> payload)
{
    // The packet payload contains:
    // TPacketGCShopStartEx shopStartPacket
    // followed by tabCount * TPacketGCShopStartEx::TSubPacketShopTab
    
    if (payload.size() < sizeof(TPacketGCShopStartEx))
    {
        return std::nullopt;
    }

    const TPacketGCShopStartEx* shopStartPacket = reinterpret_cast<const TPacketGCShopStartEx*>(payload.data());
    size_t readPoint = sizeof(TPacketGCShopStartEx);
    
    uint32_t targetId = shopStartPacket->owner_vid;
    uint8_t tabCount = shopStartPacket->shop_tab_count;
    
    // Check if we have enough data for all tabs
    if (payload.size() < readPoint + (tabCount * sizeof(TPacketGCShopStartEx::TSubPacketShopTab)))
    {
        return std::nullopt;
    }

    CPythonShop::Instance().Clear();
    CPythonShop::Instance().SetTabCount(tabCount);

    for (uint8_t i = 0; i < tabCount; ++i)
    {
        const TPacketGCShopStartEx::TSubPacketShopTab* packTab = 
            reinterpret_cast<const TPacketGCShopStartEx::TSubPacketShopTab*>(payload.data() + readPoint);
        readPoint += sizeof(TPacketGCShopStartEx::TSubPacketShopTab);

        CPythonShop::Instance().SetTabCoinType(i, packTab->coin_type);
        CPythonShop::Instance().SetTabName(i, packTab->name);

        for (uint8_t j = 0; j < SHOP_HOST_ITEM_MAX_NUM; ++j)
        {
            TShopItemData itemData;
            std::memcpy(&itemData, &packTab->items[j], sizeof(TShopItemData));
            CPythonShop::Instance().SetItemData(i, j, itemData);
        }
    }

    return ShopStartEvent{ targetId };
}
