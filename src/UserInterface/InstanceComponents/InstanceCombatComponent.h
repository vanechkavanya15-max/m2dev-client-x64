#pragma once

#include <cstdint>
#include <list>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>

#include "AffectFlagContainer.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceComponents {

/**
 * @brief Komponent zarzadzajacy mechanika walki, kolejka obrazen, flagami PVP oraz afektami.
 * 
 * Wydzielony z monolitu CInstanceBase (wczesniej w InstanceBaseBattle.cpp).
 */
class InstanceCombatComponent {
public:
    struct DamageRecord {
        uint32_t damage{0};
        uint8_t flag{0};
        bool isSelf{false};
        bool isTarget{false};
    };

    InstanceCombatComponent() = default;
    ~InstanceCombatComponent() = default;

    InstanceCombatComponent(const InstanceCombatComponent&) = delete;
    InstanceCombatComponent& operator=(const InstanceCombatComponent&) = delete;

    // Kolejka obrazen
    void AddDamage(uint32_t damage, uint8_t flag, bool isSelf, bool isTarget);
    [[nodiscard]] std::list<DamageRecord> PopAllDamages();
    [[nodiscard]] size_t GetPendingDamageCount() const noexcept;
    void ClearDamages();

    // Flagi i typ combo
    void SetComboType(uint32_t comboType) noexcept { m_comboType = comboType; }
    [[nodiscard]] uint32_t GetComboType() const noexcept { return m_comboType; }
    void SetAttackSpeed(uint32_t attackSpeed) noexcept { m_attackSpeed = attackSpeed; }
    [[nodiscard]] uint32_t GetAttackSpeed() const noexcept { return m_attackSpeed; }

    // Flagi afektow (otrucie, spowolnienie, ogłuszenie, aury itp.)
    void SetAffectFlags(const CAffectFlagContainer& flags) noexcept { m_affectFlags = flags; }
    [[nodiscard]] const CAffectFlagContainer& GetAffectFlags() const noexcept { return m_affectFlags; }
    CAffectFlagContainer& GetAffectFlagsRef() noexcept { return m_affectFlags; }

    // Stan walki i smierci
    void SetDead(bool isDead) noexcept { m_isDead = isDead; }
    [[nodiscard]] bool IsDead() const noexcept { return m_isDead; }
    void SetStun(bool isStunned) noexcept { m_isStunned = isStunned; }
    [[nodiscard]] bool IsStunned() const noexcept { return m_isStunned; }

    // Rejestr kluczy PVP/GVG (Thread-safe)
    static void InsertPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void RemovePVPKey(uint32_t srcVid, uint32_t dstVid);
    static bool HasPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void ClearPVPKeys();

    void Clear();

private:
    std::list<DamageRecord> m_damageQueue;
    mutable std::shared_mutex m_damageMutex;

    CAffectFlagContainer m_affectFlags;
    uint32_t m_comboType{0};
    uint32_t m_attackSpeed{100};
    bool m_isDead{false};
    bool m_isStunned{false};

    static inline std::unordered_set<uint64_t> ms_pvpKeys;
    static inline std::shared_mutex ms_pvpMutex;

    static constexpr uint64_t MakePVPKey(uint32_t srcVid, uint32_t dstVid) noexcept {
        return (static_cast<uint64_t>(srcVid) << 32) | static_cast<uint64_t>(dstVid);
    }
};

} // namespace UserInterface::InstanceComponents
