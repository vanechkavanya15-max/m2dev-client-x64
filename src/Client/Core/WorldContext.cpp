#include "WorldContext.h"
#include "DomainEvents.h"
#include "EventBus.h"
#include <algorithm>

namespace Client::Core {

WorldContext::WorldContext()
    : spatialGrid(1024.0f)
{
}

void WorldContext::Reset() noexcept {
    std::unique_lock lock(m_contextMutex);
    currentHp = 0;
    maxHp = 0;
    currentSp = 0;
    maxSp = 0;
    currentExp = 0;
    currentGold = 0;
    currentCheque = 0;
    currentGaya = 0;
    currentMapIndex = 0;
    currentChannel = 1;
    isDead = false;

    localPlayerVid = EntityVid{0};
    localPlayerCoords = MapCoords{};
    localPlayerRotation = 0.0f;
    posX = 0.0f;
    posY = 0.0f;
    posZ = 0.0f;

    entities.clear();
    points.fill(0);

    // Czyszczenie poszczegolnych domen
    inventory.Clear();
    stats.Reset();
    skills.Clear();
    quickslot.Clear();
    actors.Clear();
    spatialGrid.Clear();
}

int64_t WorldContext::GetPoint(uint32_t pointType) const noexcept {
    std::shared_lock lock(m_contextMutex);
    if (pointType >= points.size()) return 0;
    return points[pointType];
}

void WorldContext::SetPoint(uint32_t pointType, int64_t value) noexcept {
    std::unique_lock lock(m_contextMutex);
    if (pointType < points.size()) {
        points[pointType] = value;
    }
    stats.SetPoint(pointType, value);

    switch (pointType) {
        case 5: { // POINT_HP
            currentHp = static_cast<uint32_t>(std::max<int64_t>(0, value));
            isDead = (currentHp == 0);
            if (localPlayerVid.get() != 0) {
                actors.SetDead(EterBase::EntityId{localPlayerVid.get()}, isDead);
            }
            break;
        }
        case 6: { // POINT_MAX_HP
            maxHp = static_cast<uint32_t>(std::max<int64_t>(0, value));
            break;
        }
        case 7: { // POINT_SP
            currentSp = static_cast<uint32_t>(std::max<int64_t>(0, value));
            break;
        }
        case 8: { // POINT_MAX_SP
            maxSp = static_cast<uint32_t>(std::max<int64_t>(0, value));
            break;
        }
        case 3: { // POINT_EXP
            currentExp = static_cast<uint64_t>(std::max<int64_t>(0, value));
            break;
        }
        case 11: { // POINT_GOLD
            int64_t oldGold = currentGold;
            currentGold = value;
            ::UserInterface::Core::EventBus::GetInstance().Publish(
                ::Client::Core::PlayerGoldUpdatedEvent{ oldGold, currentGold }
            );
            break;
        }
        default:
            break;
    }
}

void WorldContext::SetPlayerHp(uint32_t hp, std::optional<uint32_t> maxHpVal) noexcept {
    std::unique_lock lock(m_contextMutex);
    currentHp = hp;
    isDead = (hp == 0);
    points[5] = hp;
    stats.SetPoint(5, hp);

    if (maxHpVal.has_value()) {
        maxHp = *maxHpVal;
        points[6] = *maxHpVal;
        stats.SetPoint(6, *maxHpVal);
    }

    if (localPlayerVid.get() != 0) {
        actors.SetDead(EterBase::EntityId{localPlayerVid.get()}, isDead);
        if (isDead) {
            ::UserInterface::Core::EventBus::GetInstance().Publish(
                ::Client::Core::ActorDeadEvent{ localPlayerVid.get() }
            );
        }
    }
}

void WorldContext::SetPlayerSp(uint32_t sp, std::optional<uint32_t> maxSpVal) noexcept {
    std::unique_lock lock(m_contextMutex);
    currentSp = sp;
    points[7] = sp;
    stats.SetPoint(7, sp);

    if (maxSpVal.has_value()) {
        maxSp = *maxSpVal;
        points[8] = *maxSpVal;
        stats.SetPoint(8, *maxSpVal);
    }
}

void WorldContext::SetPlayerDead(bool dead) noexcept {
    std::unique_lock lock(m_contextMutex);
    isDead = dead;
    if (dead) {
        currentHp = 0;
        points[5] = 0;
        stats.SetPoint(5, 0);
    }

    if (localPlayerVid.get() != 0) {
        actors.SetDead(EterBase::EntityId{localPlayerVid.get()}, dead);
        if (dead) {
            ::UserInterface::Core::EventBus::GetInstance().Publish(
                ::Client::Core::ActorDeadEvent{ localPlayerVid.get() }
            );
        }
    }
}

bool WorldContext::IsAlive() const noexcept {
    std::shared_lock lock(m_contextMutex);
    return !isDead && currentHp > 0;
}

void WorldContext::SetLocalPlayer(EntityVid vid, float x, float y, float z, float rotation, const std::string& name) {
    std::unique_lock lock(m_contextMutex);
    localPlayerVid = vid;
    posX = x;
    posY = y;
    posZ = z;
    localPlayerRotation = rotation;
    localPlayerCoords = MapCoords{x, y, z};

    EterBase::EntityId eid{vid.get()};
    actors.SetMainActorVid(eid);

    Client::World::ActorRecord record{
        .vid = eid,
        .race = 0,
        .type = 0, // PC
        .x = x,
        .y = y,
        .z = z,
        .rotation = rotation,
        .name = name.empty() ? "LocalPlayer" : name,
        .guildId = 0,
        .empire = 1,
        .isDead = isDead
    };
    actors.RegisterActor(record);
    spatialGrid.Insert(eid, x, y);
}

void WorldContext::UpdatePlayerPosition(float x, float y, float z, float rotation) noexcept {
    std::unique_lock lock(m_contextMutex);
    posX = x;
    posY = y;
    posZ = z;
    localPlayerRotation = rotation;
    localPlayerCoords = MapCoords{x, y, z};

    if (localPlayerVid.get() != 0) {
        EterBase::EntityId eid{localPlayerVid.get()};
        actors.UpdatePosition(eid, x, y, z, rotation);
        spatialGrid.Update(eid, x, y);
    }
}

bool WorldContext::RegisterActor(const Client::World::ActorRecord& record) {
    std::unique_lock lock(m_contextMutex);
    bool ok = actors.RegisterActor(record);
    if (ok) {
        spatialGrid.Insert(EterBase::EntityId{record.vid.get()}, record.x, record.y);
    }
    return ok;
}

bool WorldContext::UnregisterActor(EntityVid vid) {
    std::unique_lock lock(m_contextMutex);
    bool ok = actors.UnregisterActor(EterBase::EntityId{vid.get()});
    if (ok) {
        spatialGrid.Remove(EterBase::EntityId{vid.get()});
    }
    return ok;
}

bool WorldContext::UpdateActorPosition(EntityVid vid, float x, float y, float z, float rotation) {
    std::unique_lock lock(m_contextMutex);
    bool ok = actors.UpdatePosition(EterBase::EntityId{vid.get()}, x, y, z, rotation);
    if (ok) {
        spatialGrid.Update(EterBase::EntityId{vid.get()}, x, y);
        if (vid == localPlayerVid) {
            posX = x;
            posY = y;
            posZ = z;
            localPlayerRotation = rotation;
            localPlayerCoords = MapCoords{x, y, z};
        }
    }
    return ok;
}

std::vector<Client::World::ActorRecord> WorldContext::FindActorsInRadius(float x, float y, float radius) const {
    std::shared_lock lock(m_contextMutex);
    auto vids = spatialGrid.QueryRadius(x, y, radius);
    std::vector<Client::World::ActorRecord> result;
    result.reserve(vids.size());
    for (const auto& vid : vids) {
        auto actor = actors.GetActor(EterBase::EntityId{vid.get()});
        if (actor.has_value()) {
            result.push_back(std::move(*actor));
        }
    }
    return result;
}

std::vector<Client::World::ActorRecord> WorldContext::FindNearbyActors(float radius) const {
    std::shared_lock lock(m_contextMutex);
    auto vids = spatialGrid.QueryRadius(posX, posY, radius);
    std::vector<Client::World::ActorRecord> result;
    result.reserve(vids.size());
    for (const auto& vid : vids) {
        auto actor = actors.GetActor(EterBase::EntityId{vid.get()});
        if (actor.has_value()) {
            result.push_back(std::move(*actor));
        }
    }
    return result;
}

std::optional<Client::World::ActorRecord> WorldContext::FindNearestActor(float maxRadius, bool excludeSelf) const {
    std::shared_lock lock(m_contextMutex);
    std::optional<EterBase::EntityId> ignore = std::nullopt;
    if (excludeSelf && localPlayerVid.get() != 0) {
        ignore = EterBase::EntityId{localPlayerVid.get()};
    }
    auto nearestVid = spatialGrid.QueryNearest(posX, posY, maxRadius, ignore);
    if (!nearestVid.has_value()) {
        return std::nullopt;
    }
    return actors.GetActor(EterBase::EntityId{nearestVid->get()});
}

bool WorldContext::CanUseSkill(uint32_t skillId) const {
    std::shared_lock lock(m_contextMutex);
    if (isDead || currentHp == 0) {
        return false;
    }
    const auto sid = Client::Gameplay::SkillId{skillId};
    if (!skills.HasSkill(sid)) {
        return false;
    }
    if (!skills.IsSkillReady(sid)) {
        return false;
    }
    uint32_t spCost = skills.CalculateSPCost(sid);
    return currentSp >= spCost;
}

bool WorldContext::UseSkill(uint32_t skillId, std::chrono::milliseconds cooldownDuration) {
    std::unique_lock lock(m_contextMutex);
    if (isDead || currentHp == 0) {
        return false;
    }
    const auto sid = Client::Gameplay::SkillId{skillId};
    if (!skills.HasSkill(sid)) {
        return false;
    }
    if (!skills.IsSkillReady(sid)) {
        return false;
    }
    uint32_t spCost = skills.CalculateSPCost(sid);
    if (currentSp < spCost) {
        return false;
    }

    currentSp -= spCost;
    points[7] = currentSp;
    stats.SetPoint(7, currentSp);

    skills.StartCooldown(sid, cooldownDuration);
    return true;
}

bool WorldContext::UseSkillMs(uint32_t skillId, uint32_t cooldownMs) {
    return UseSkill(skillId, std::chrono::milliseconds{cooldownMs});
}

bool WorldContext::BindQuickslotSkill(uint32_t quickslotIndex, uint32_t skillId) {
    std::unique_lock lock(m_contextMutex);
    const auto sid = Client::Gameplay::SkillId{skillId};
    if (!skills.HasSkill(sid)) {
        return false;
    }
    Client::Gameplay::QuickslotItem item{
        .type = 2, // Typ 2: Umiejetnosc (Skill)
        .position = static_cast<uint8_t>(skillId & 0xFF)
    };
    auto res = quickslot.SetSlot(quickslotIndex, item);
    return res.has_value();
}

bool WorldContext::BindQuickslotItem(uint32_t quickslotIndex, uint16_t inventorySlot) {
    std::unique_lock lock(m_contextMutex);
    auto itemRes = inventory.GetItem(Client::Gameplay::InventoryWindow::Inventory, EterBase::ItemSlot{inventorySlot});
    if (!itemRes.has_value() || itemRes->vnum.get() == 0) {
        return false;
    }
    Client::Gameplay::QuickslotItem item{
        .type = 1, // Typ 1: Przedmiot (Item)
        .position = static_cast<uint8_t>(inventorySlot & 0xFF)
    };
    auto res = quickslot.SetSlot(quickslotIndex, item);
    return res.has_value();
}

} // namespace Client::Core
