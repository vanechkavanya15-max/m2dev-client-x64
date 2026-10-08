#include "../../StdAfx.h"
#include "ShopHandler.h"
#include "Client/Gameplay/TradeDomain.h"
#include "UserInterface/Packet.h"
#include "UserInterface/Packets/Packet_Shop.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ShopHandler::HandleShopPacket(
    std::span<const uint8_t> payload, 
    Client::Gameplay::NpcShop& shop_domain)
{
    if (payload.size() < sizeof(TPacketGCShop)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCShop header;
    std::memcpy(&header, payload.data(), sizeof(TPacketGCShop));

    switch (header.subheader) {
        case ShopSub::GC::START: {
            constexpr size_t expected_size = sizeof(TPacketGCShop) + sizeof(uint32_t) + sizeof(TPacketGCShopStart);
            if (payload.size() < expected_size) {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            TPacketGCShopStart start_packet;
            std::memcpy(&start_packet, payload.data() + sizeof(TPacketGCShop) + sizeof(uint32_t), sizeof(TPacketGCShopStart));

            for (uint8_t i = 0; i < SHOP_HOST_ITEM_MAX_NUM; ++i) {
                const auto& item_data = start_packet.items[i];
                if (item_data.vnum != 0) {
                    Client::Gameplay::ShopItem domain_item{
                        EterBase::ItemVnum(item_data.vnum),
                        item_data.count,
                        Client::Gameplay::Price(item_data.price),
                        Client::Gameplay::Price(item_data.price) // Setting same price for sell as fallback or placeholder
                    };
                    shop_domain.RegisterItem(domain_item);
                }
            }
            return {};
        }

        case ShopSub::GC::UPDATE_ITEM: {
            constexpr size_t expected_size = sizeof(TPacketGCShop) + sizeof(TPacketGCShopUpdateItem);
            if (payload.size() < expected_size) {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            TPacketGCShopUpdateItem update_packet;
            std::memcpy(&update_packet, payload.data() + sizeof(TPacketGCShop), sizeof(TPacketGCShopUpdateItem));

            if (update_packet.item.vnum != 0) {
                Client::Gameplay::ShopItem domain_item{
                    EterBase::ItemVnum(update_packet.item.vnum),
                    update_packet.item.count,
                    Client::Gameplay::Price(update_packet.item.price),
                    Client::Gameplay::Price(update_packet.item.price)
                };
                shop_domain.RegisterItem(domain_item);
            }
            return {};
        }

        case ShopSub::GC::END: {
            // Nothing to parse further, END indicates successful closure or setup
            return {};
        }

        default:
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }
}

} // namespace Client::Network::Handlers
