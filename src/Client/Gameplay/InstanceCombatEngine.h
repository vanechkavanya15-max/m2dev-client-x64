#pragma once

#include <cstdint>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <unordered_set>
#include <chrono>

namespace Client::Gameplay {

enum class DamageFlag : uint8_t {
    Normal    = 0,
    Poison    = 1 << 0,
    Dodge     = 1 << 1,
    Block     = 1 << 2,
    Penetrate = 1 << 3,
    Critical  = 1 << 4
};

constexpr DamageFlag operator|(DamageFlag lhs, DamageFlag rhs) {
    return static_cast<DamageFlag>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr DamageFlag operator&(DamageFlag lhs, DamageFlag rhs) {
    return static_cast<DamageFlag>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr DamageFlag& operator|=(DamageFlag& lhs, DamageFlag rhs) {
    lhs = lhs | rhs;
    return lhs;
}

struct DamageRecord {
    uint32_t damage;
    DamageFlag flags;
    bool isSelf;
    bool isTarget;
    uint64_t timestampMs;
};

class InstanceCombatEngine {
public:
    InstanceCombatEngine() = default;
    ~InstanceCombatEngine() = default;

    // Delete copy and move semantics due to mutex
    InstanceCombatEngine(const InstanceCombatEngine&) = delete;
    InstanceCombatEngine& operator=(const InstanceCombatEngine&) = delete;
    InstanceCombatEngine(InstanceCombatEngine&&) = delete;
    InstanceCombatEngine& operator=(InstanceCombatEngine&&) = delete;

    void AddDamage(uint32_t damage, DamageFlag flags, bool isSelf, bool isTarget);
    [[nodiscard]] std::vector<DamageRecord> PopAllDamages();
    [[nodiscard]] size_t GetPendingDamageCount() const noexcept;
    void ClearDamages();

    // PVP Key registry
    static void InsertPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void RemovePVPKey(uint32_t srcVid, uint32_t dstVid);
    static bool HasPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void ClearPVPKeys();

private:
    std::vector<DamageRecord> pendingDamages;
    mutable std::shared_mutex damagesMutex;

    // Static registry variables
    static std::unordered_set<uint64_t> pvpKeys;
    static std::shared_mutex pvpKeysMutex;

    static uint64_t MakePVPKey(uint32_t srcVid, uint32_t dstVid) noexcept {
        return (static_cast<uint64_t>(srcVid) << 32) | static_cast<uint64_t>(dstVid);
    }
};

} // namespace Client::Gameplay
