#pragma once

#include <span>
#include <cstdint>
#include <cstring>
#include "EterBase/Result.h"
#include "Client/Network/Protocol/Protocol.h"
#include "UserInterface/Domain/ShopInventoryModel.h"

namespace Client::Network::Handlers {

/**
 * @brief Zero-Conflict C++23 handler for NPC shop packets.
 * 
 * Safely parses START and START_EX packets checking boundaries via std::span
 * and providing deterministic outcomes via std::expected.
 */
class ShopOpenPacketHandler {
public:
    struct ShopOpenContext {
        uint32_t ownerVid{0};
    };

    static EterBase::PacketResult<ShopOpenContext> HandleShopStart(
        std::span<const uint8_t> payload, 
        UserInterface::Domain::ShopInventoryModel& shopModel) noexcept
    {
        if (payload.size() < sizeof(TPacketGCShop)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        constexpr size_t expected_size = sizeof(TPacketGCShop) + sizeof(uint32_t) + sizeof(TPacketGCShopStart);
        if (payload.size() < expected_size) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        uint32_t ownerVid;
        std::memcpy(&ownerVid, payload.data() + sizeof(TPacketGCShop), sizeof(uint32_t));

        TPacketGCShopStart start_packet;
        std::memcpy(&start_packet, payload.data() + sizeof(TPacketGCShop) + sizeof(uint32_t), sizeof(TPacketGCShopStart));

        shopModel = UserInterface::Domain::ShopInventoryModel(); 
        
        shopModel.addTab("Shop", 0, SHOP_HOST_ITEM_MAX_NUM);

        for (uint8_t i = 0; i < SHOP_HOST_ITEM_MAX_NUM; ++i) {
            const auto& itemData = start_packet.items[i];
            if (itemData.vnum != 0) {
                UserInterface::Domain::ShopItemData domainItem{};
                domainItem.vnum = EterBase::ItemVnum(itemData.vnum);
                domainItem.count = itemData.count;
                domainItem.price = itemData.price;
                
                for (size_t s = 0; s < ITEM_SOCKET_SLOT_MAX_NUM; ++s) {
                    domainItem.sockets.push_back(itemData.alSockets[s]);
                }
                
                for (size_t a = 0; a < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++a) {
                    domainItem.attributes.push_back(UserInterface::Domain::ShopItemAttribute{
                        static_cast<uint16_t>(itemData.aAttr[a].bType), 
                        itemData.aAttr[a].sValue
                    });
                }
                
                auto res = shopModel.setItem(0, EterBase::ItemSlot(i), domainItem);
                if (!res.has_value()) {
                     return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                }
            }
        }

        return ShopOpenContext{ownerVid};
    }

    static EterBase::PacketResult<ShopOpenContext> HandleShopStartEx(
        std::span<const uint8_t> payload, 
        UserInterface::Domain::ShopInventoryModel& shopModel) noexcept
    {
        if (payload.size() < sizeof(TPacketGCShop)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        size_t readPos = sizeof(TPacketGCShop);

        if (payload.size() < readPos + sizeof(TPacketGCShopStartEx)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCShopStartEx startExPacket;
        std::memcpy(&startExPacket, payload.data() + readPos, sizeof(TPacketGCShopStartEx));
        readPos += sizeof(TPacketGCShopStartEx);

        uint32_t ownerVid = startExPacket.owner_vid;
        uint8_t tabCount = startExPacket.shop_tab_count;

        if (payload.size() < readPos + (tabCount * sizeof(TPacketGCShopStartEx::TSubPacketShopTab))) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        shopModel = UserInterface::Domain::ShopInventoryModel(); 

        for (uint8_t t = 0; t < tabCount; ++t) {
            TPacketGCShopStartEx::TSubPacketShopTab tabPacket;
            std::memcpy(&tabPacket, payload.data() + readPos, sizeof(TPacketGCShopStartEx::TSubPacketShopTab));
            readPos += sizeof(TPacketGCShopStartEx::TSubPacketShopTab);

            std::string tabName(tabPacket.name, strnlen(tabPacket.name, sizeof(tabPacket.name)));
            shopModel.addTab(tabName, tabPacket.coin_type, SHOP_HOST_ITEM_MAX_NUM);

            for (uint8_t i = 0; i < SHOP_HOST_ITEM_MAX_NUM; ++i) {
                const auto& itemData = tabPacket.items[i];
                if (itemData.vnum != 0) {
                    UserInterface::Domain::ShopItemData domainItem{};
                    domainItem.vnum = EterBase::ItemVnum(itemData.vnum);
                    domainItem.count = itemData.count;
                    domainItem.price = itemData.price;
                    
                    for (size_t s = 0; s < ITEM_SOCKET_SLOT_MAX_NUM; ++s) {
                        domainItem.sockets.push_back(itemData.alSockets[s]);
                    }
                    
                    for (size_t a = 0; a < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++a) {
                        domainItem.attributes.push_back(UserInterface::Domain::ShopItemAttribute{
                            static_cast<uint16_t>(itemData.aAttr[a].bType), 
                            itemData.aAttr[a].sValue
                        });
                    }
                    
                    auto res = shopModel.setItem(t, EterBase::ItemSlot(i), domainItem);
                    if (!res.has_value()) {
                        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                    }
                }
            }
        }

        return ShopOpenContext{ownerVid};
    }
    
    static EterBase::PacketResult<ShopOpenContext> DispatchShopStart(
        std::span<const uint8_t> payload, 
        UserInterface::Domain::ShopInventoryModel& shopModel) noexcept
    {
        if (payload.size() < sizeof(TPacketGCShop)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }
        
        TPacketGCShop header;
        std::memcpy(&header, payload.data(), sizeof(TPacketGCShop));
        
        if (header.subheader == ShopSub::GC::START) {
            return HandleShopStart(payload, shopModel);
        } else if (header.subheader == ShopSub::GC::START_EX) {
            return HandleShopStartEx(payload, shopModel);
        }
        
        return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }
};

} // namespace Client::Network::Handlers
