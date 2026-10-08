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
    for (size_t i = 0; i < ctx.inventory.size(); ++i) {
        const auto& item = ctx.inventory[i];
        if (i > 0) {
            itemsJson += ",";
        }
        itemsJson += std::format(
            "{{\"slot\":{},\"vnum\":{},\"count\":{}}}",
            item.slot, item.vnum, item.count
        );
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
