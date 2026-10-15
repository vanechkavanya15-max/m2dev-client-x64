#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <span>

// ==========================================
// Mock necessary domain and packet types
// ==========================================
namespace EterBase {
    template <typename Tag, typename Underlying = uint32_t, Underlying DefaultValue = Underlying{}>
    class StrongType {
    public:
        constexpr StrongType() noexcept : m_value(DefaultValue) {}
        constexpr explicit StrongType(Underlying value) noexcept : m_value(value) {}
        [[nodiscard]] constexpr Underlying value() const noexcept { return m_value; }
        [[nodiscard]] constexpr Underlying get() const noexcept { return m_value; }
        constexpr bool operator==(const StrongType& o) const noexcept { return m_value == o.m_value; }
    private:
        Underlying m_value;
    };

    struct ItemVnumTag {};
    using ItemVnum = StrongType<ItemVnumTag, uint32_t, 0>;

    enum class PacketError : uint8_t {
        None = 0,
        BufferUnderflow,
        InvalidHeader,
        UnknownOpcode
    };

    template <typename T = void>
    struct PacketResult {
        PacketError error{PacketError::None};
        bool has_value() const { return error == PacketError::None; }
        PacketError error_value() const { return error; }
    };

    template <typename E>
    PacketResult<void> MakeError(E err) {
        return {static_cast<PacketError>(err)};
    }
}

namespace Client::Gameplay {
    struct PriceTag {};
    using Price = EterBase::StrongType<PriceTag, uint64_t, 0>;

    struct ShopItem {
        EterBase::ItemVnum vnum;
        uint8_t count;
        Price buy_price;
        Price sell_price;
    };

    class NpcShop {
    public:
        void RegisterItem(const ShopItem& item) {
            items.push_back(item);
        }
        std::vector<ShopItem> items;
    };
}

#define SHOP_HOST_ITEM_MAX_NUM 40

struct TShopItemData {
    uint32_t vnum;
    uint32_t price;
    uint8_t count;
    uint8_t displayPos;
    int32_t sockets[3];
    int16_t attr[7][2];
};

namespace ShopSub::GC {
    constexpr uint8_t START = 0;
    constexpr uint8_t END = 1;
    constexpr uint8_t UPDATE_ITEM = 2;
}

struct TPacketGCShop {
    uint16_t header;
    uint16_t length;
    uint8_t subheader;
};

struct TPacketGCShopStart {
    TShopItemData items[SHOP_HOST_ITEM_MAX_NUM];
};

struct TPacketGCShopUpdateItem {
    uint8_t pos;
    TShopItemData item;
};

// ==========================================
// We include the ACTUAL logic being tested
// by compiling it inline for the test since 
// we mocked its dependencies above.
// ==========================================
namespace Client::Network::Handlers {
    class ShopHandler {
    public:
        static EterBase::PacketResult<void> HandleShopPacket(
            std::span<const uint8_t> payload, 
            Client::Gameplay::NpcShop& shop_domain);
    };

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
                        Client::Gameplay::Price(item_data.price)
                    };
                    shop_domain.RegisterItem(domain_item);
                }
            }
            return {EterBase::PacketError::None};
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
            return {EterBase::PacketError::None};
        }

        case ShopSub::GC::END: {
            return {EterBase::PacketError::None};
        }

        default:
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }
}
}

// ==========================================
// Tests
// ==========================================

void test_shop_start() {
    Client::Gameplay::NpcShop shop;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCShop) + sizeof(uint32_t) + sizeof(TPacketGCShopStart), 0);
    
    TPacketGCShop* header = reinterpret_cast<TPacketGCShop*>(buffer.data());
    header->subheader = ShopSub::GC::START;
    
    TPacketGCShopStart* start_packet = reinterpret_cast<TPacketGCShopStart*>(buffer.data() + sizeof(TPacketGCShop) + sizeof(uint32_t));
    start_packet->items[0].vnum = 1234;
    start_packet->items[0].price = 5000;
    start_packet->items[0].count = 10;
    
    start_packet->items[15].vnum = 5678;
    start_packet->items[15].price = 1000;
    start_packet->items[15].count = 1;

    auto res = Client::Network::Handlers::ShopHandler::HandleShopPacket(buffer, shop);
    assert(res.has_value());
    assert(shop.items.size() == 2);
    assert(shop.items[0].vnum.get() == 1234);
    assert(shop.items[0].buy_price.get() == 5000);
    assert(shop.items[0].count == 10);
    
    assert(shop.items[1].vnum.get() == 5678);
    assert(shop.items[1].buy_price.get() == 1000);
    assert(shop.items[1].count == 1);
    std::cout << "[OK] test_shop_start" << std::endl;
}

void test_shop_update_item() {
    Client::Gameplay::NpcShop shop;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCShop) + sizeof(TPacketGCShopUpdateItem), 0);
    
    TPacketGCShop* header = reinterpret_cast<TPacketGCShop*>(buffer.data());
    header->subheader = ShopSub::GC::UPDATE_ITEM;
    
    TPacketGCShopUpdateItem* update_packet = reinterpret_cast<TPacketGCShopUpdateItem*>(buffer.data() + sizeof(TPacketGCShop));
    update_packet->pos = 5;
    update_packet->item.vnum = 999;
    update_packet->item.price = 250;
    update_packet->item.count = 50;

    auto res = Client::Network::Handlers::ShopHandler::HandleShopPacket(buffer, shop);
    assert(res.has_value());
    assert(shop.items.size() == 1);
    assert(shop.items[0].vnum.get() == 999);
    assert(shop.items[0].buy_price.get() == 250);
    assert(shop.items[0].count == 50);
    std::cout << "[OK] test_shop_update_item" << std::endl;
}

void test_shop_end() {
    Client::Gameplay::NpcShop shop;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCShop), 0);
    TPacketGCShop* header = reinterpret_cast<TPacketGCShop*>(buffer.data());
    header->subheader = ShopSub::GC::END;

    auto res = Client::Network::Handlers::ShopHandler::HandleShopPacket(buffer, shop);
    assert(res.has_value());
    assert(shop.items.empty());
    std::cout << "[OK] test_shop_end" << std::endl;
}

void test_buffer_underflow() {
    Client::Gameplay::NpcShop shop;
    
    std::vector<uint8_t> buffer1(1, 0); // Too small even for header
    auto res1 = Client::Network::Handlers::ShopHandler::HandleShopPacket(buffer1, shop);
    assert(!res1.has_value());
    assert(res1.error_value() == EterBase::PacketError::BufferUnderflow);

    std::vector<uint8_t> buffer2(sizeof(TPacketGCShop) + 1, 0); // Header + incomplete payload
    TPacketGCShop* header2 = reinterpret_cast<TPacketGCShop*>(buffer2.data());
    header2->subheader = ShopSub::GC::START;
    
    auto res2 = Client::Network::Handlers::ShopHandler::HandleShopPacket(buffer2, shop);
    assert(!res2.has_value());
    assert(res2.error_value() == EterBase::PacketError::BufferUnderflow);
    
    std::cout << "[OK] test_buffer_underflow" << std::endl;
}

int main() {
    test_shop_start();
    test_shop_update_item();
    test_shop_end();
    test_buffer_underflow();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
