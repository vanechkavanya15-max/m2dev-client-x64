#include "InstanceCombatEngine.h"

namespace Client::Gameplay {

std::unordered_set<uint64_t> InstanceCombatEngine::pvpKeys;
std::shared_mutex InstanceCombatEngine::pvpKeysMutex;

void InstanceCombatEngine::AddDamage(uint32_t damage, DamageFlag flags, bool isSelf, bool isTarget) {
    auto now = std::chrono::steady_clock::now();
    uint64_t timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::unique_lock lock(damagesMutex);
    pendingDamages.push_back(DamageRecord{damage, flags, isSelf, isTarget, timestampMs});
}

std::vector<DamageRecord> InstanceCombatEngine::PopAllDamages() {
    std::unique_lock lock(damagesMutex);
    std::vector<DamageRecord> result;
    std::swap(result, pendingDamages);
    return result;
}

size_t InstanceCombatEngine::GetPendingDamageCount() const noexcept {
    std::shared_lock lock(damagesMutex);
    return pendingDamages.size();
}

void InstanceCombatEngine::ClearDamages() {
    std::unique_lock lock(damagesMutex);
    pendingDamages.clear();
}

void InstanceCombatEngine::InsertPVPKey(uint32_t srcVid, uint32_t dstVid) {
    uint64_t key = MakePVPKey(srcVid, dstVid);
    std::unique_lock lock(pvpKeysMutex);
    pvpKeys.insert(key);
}

void InstanceCombatEngine::RemovePVPKey(uint32_t srcVid, uint32_t dstVid) {
    uint64_t key = MakePVPKey(srcVid, dstVid);
    std::unique_lock lock(pvpKeysMutex);
    pvpKeys.erase(key);
}

bool InstanceCombatEngine::HasPVPKey(uint32_t srcVid, uint32_t dstVid) {
    uint64_t key = MakePVPKey(srcVid, dstVid);
    std::shared_lock lock(pvpKeysMutex);
    return pvpKeys.contains(key);
}

void InstanceCombatEngine::ClearPVPKeys() {
    std::unique_lock lock(pvpKeysMutex);
    pvpKeys.clear();
}

} // namespace Client::Gameplay
