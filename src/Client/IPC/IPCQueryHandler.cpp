#include "IPCQueryHandler.h"
#include "../Core/WorldContext.h"
#include <format>
#include <cmath>

namespace Client::IPC {

std::string IPCQueryHandler::HandleQueryPlayerState(Client::Core::WorldContext& ctx) {
    return std::format(
        "{{\"hp\":{},\"max_hp\":{},\"sp\":{},\"max_sp\":{},\"exp\":{},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}},\"is_dead\":{}}}",
        ctx.currentHp, ctx.maxHp, ctx.currentSp, ctx.maxSp, ctx.currentExp,
        ctx.posX, ctx.posY, ctx.posZ, ctx.isDead ? "true" : "false"
    );
}

std::string IPCQueryHandler::HandleQueryInventory(Client::Core::WorldContext& ctx) {
    std::string itemsJson = "[";
    bool first = true;
    for (uint16_t slotIdx = 0; slotIdx < Client::Gameplay::InventoryDomain::INVENTORY_MAX_NUM; ++slotIdx) {
        auto itemRes = ctx.inventory.GetItem(Client::Gameplay::InventoryWindow::Inventory, EterBase::ItemSlot{slotIdx});
        if (itemRes.has_value() && itemRes->vnum.get() != 0) {
            if (!first) {
                itemsJson += ",";
            }
            itemsJson += std::format(
                "{{\"slot\":{},\"vnum\":{},\"count\":{}}}",
                slotIdx, itemRes->vnum.get(), itemRes->count
            );
            first = false;
        }
    }
    itemsJson += "]";
    return "{\"inventory\":" + itemsJson + "}";
}

std::string IPCQueryHandler::HandleQuerySurroundings(Client::Core::WorldContext& ctx, float radius) {
    std::string entitiesJson = "[";
    bool first = true;
    for (const auto& entity : ctx.entities) {
        float dx = entity.x - ctx.posX;
        float dy = entity.y - ctx.posY;
        float dz = entity.z - ctx.posZ;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (dist <= radius) {
            if (!first) {
                entitiesJson += ",";
            }
            entitiesJson += std::format(
                "{{\"vid\":{},\"x\":{},\"y\":{},\"z\":{},\"is_hostile\":{}}}",
                entity.vid, entity.x, entity.y, entity.z, entity.isHostile ? "true" : "false"
            );
            first = false;
        }
    }
    entitiesJson += "]";
    return "{\"surroundings\":" + entitiesJson + "}";
}

} // namespace Client::IPC
