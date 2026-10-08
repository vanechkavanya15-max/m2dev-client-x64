#include "../src/EterBase/StdAfx.h"
#include "../src/Client/Network/InventoryCommandEncoder.h"
#include "../src/UserInterface/Packet.h"
#include "../src/EterBase/ModernLogger.h"
#include "../src/EterBase/Result.h"

#include <iostream>
#include <cstring>

EterBase::VoidResult TestEncodeUseItem()
{
    EterBase::ItemSlot slot(10);
    auto result = Client::Network::InventoryCommandEncoder::EncodeUseItem(slot);
    if (!result)
        return EterBase::MakeError("EncodeUseItem failed.");

    const auto& buffer = result.value();
    if (buffer.size() != sizeof(TPacketCGItemUse))
        return EterBase::MakeError("EncodeUseItem returned invalid buffer size.");

    const TPacketCGItemUse* packet = reinterpret_cast<const TPacketCGItemUse*>(buffer.data());
    if (packet->header != CG::ITEM_USE)
        return EterBase::MakeError("EncodeUseItem returned incorrect header.");
    if (packet->length != sizeof(TPacketCGItemUse))
        return EterBase::MakeError("EncodeUseItem returned incorrect length.");
    if (packet->pos.window_type != INVENTORY || packet->pos.cell != 10)
        return EterBase::MakeError("EncodeUseItem returned incorrect pos.");

    return {};
}

EterBase::VoidResult TestEncodeDropItem()
{
    EterBase::ItemSlot slot(15);
    uint32_t gold = 1000;
    uint8_t count = 5;

    auto result = Client::Network::InventoryCommandEncoder::EncodeDropItem(slot, gold, count);
    if (!result)
        return EterBase::MakeError("EncodeDropItem failed.");

    const auto& buffer = result.value();
    if (buffer.size() != sizeof(TPacketCGItemDrop2))
        return EterBase::MakeError("EncodeDropItem returned invalid buffer size.");

    const TPacketCGItemDrop2* packet = reinterpret_cast<const TPacketCGItemDrop2*>(buffer.data());
    if (packet->header != CG::ITEM_DROP2)
        return EterBase::MakeError("EncodeDropItem returned incorrect header.");
    if (packet->length != sizeof(TPacketCGItemDrop2))
        return EterBase::MakeError("EncodeDropItem returned incorrect length.");
    if (packet->pos.window_type != INVENTORY || packet->pos.cell != 15)
        return EterBase::MakeError("EncodeDropItem returned incorrect pos.");
    if (packet->gold != 1000)
        return EterBase::MakeError("EncodeDropItem returned incorrect gold.");
    if (packet->count != 5)
        return EterBase::MakeError("EncodeDropItem returned incorrect count.");

    return {};
}

EterBase::VoidResult TestEncodePickupItem()
{
    EterBase::EntityId vid(404);
    auto result = Client::Network::InventoryCommandEncoder::EncodePickupItem(vid);
    if (!result)
        return EterBase::MakeError("EncodePickupItem failed.");

    const auto& buffer = result.value();
    if (buffer.size() != sizeof(TPacketCGItemPickUp))
        return EterBase::MakeError("EncodePickupItem returned invalid buffer size.");

    const TPacketCGItemPickUp* packet = reinterpret_cast<const TPacketCGItemPickUp*>(buffer.data());
    if (packet->header != CG::ITEM_PICKUP)
        return EterBase::MakeError("EncodePickupItem returned incorrect header.");
    if (packet->length != sizeof(TPacketCGItemPickUp))
        return EterBase::MakeError("EncodePickupItem returned incorrect length.");
    if (packet->vid != 404)
        return EterBase::MakeError("EncodePickupItem returned incorrect vid.");

    return {};
}

int main()
{
    EterBase::ModernLogger::Info("Starting test_c26_inventory_command_encoder...");
    int errors = 0;

    auto res1 = TestEncodeUseItem();
    if (!res1) {
        EterBase::ModernLogger::Error("TestEncodeUseItem failed: {}", res1.error());
        errors++;
    } else {
        EterBase::ModernLogger::Info("TestEncodeUseItem PASSED.");
    }

    auto res2 = TestEncodeDropItem();
    if (!res2) {
        EterBase::ModernLogger::Error("TestEncodeDropItem failed: {}", res2.error());
        errors++;
    } else {
        EterBase::ModernLogger::Info("TestEncodeDropItem PASSED.");
    }

    auto res3 = TestEncodePickupItem();
    if (!res3) {
        EterBase::ModernLogger::Error("TestEncodePickupItem failed: {}", res3.error());
        errors++;
    } else {
        EterBase::ModernLogger::Info("TestEncodePickupItem PASSED.");
    }

    if (errors > 0) {
        EterBase::ModernLogger::Error("Some tests failed. Errors: {}", errors);
        return 1;
    }

    EterBase::ModernLogger::Info("All tests passed successfully.");
    return 0;
}
