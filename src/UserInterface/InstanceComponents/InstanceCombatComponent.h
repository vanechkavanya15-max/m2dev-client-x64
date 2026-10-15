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
 * @brief Komponent zarzadzajacy mechanika walki, kolejka obrazen, flagami PVP, pojedynkami oraz afektami.
 * 
 * Wydzielony z monolitu CInstanceBase (wczesniej w InstanceBaseBattle.cpp oraz InstanceBaseEffect.cpp).
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
    bool PopDamage(DamageRecord& outRecord);
    [[nodiscard]] size_t GetPendingDamageCount() const noexcept;
    [[nodiscard]] bool HasDamages() const noexcept;
    void ClearDamages();

    // Flagi i typ combo
    void SetComboType(uint32_t comboType) noexcept { m_comboType = comboType; }
    [[nodiscard]] uint32_t GetComboType() const noexcept { return m_comboType; }
    void SetAttackSpeed(uint32_t attackSpeed) noexcept { m_attackSpeed = attackSpeed; }
    [[nodiscard]] uint32_t GetAttackSpeed() const noexcept { return m_attackSpeed; }

    // Flagi afektow (otrucie, spowolnienie, ogluszenie, aury itp.)
    void SetAffectFlags(const CAffectFlagContainer& flags) noexcept { m_affectFlags = flags; }
    [[nodiscard]] const CAffectFlagContainer& GetAffectFlags() const noexcept { return m_affectFlags; }
    CAffectFlagContainer& GetAffectFlagsRef() noexcept { return m_affectFlags; }

    // Stan walki i smierci
    void SetDead(bool isDead) noexcept { m_isDead = isDead; }
    [[nodiscard]] bool IsDead() const noexcept { return m_isDead; }
    void SetStun(bool isStunned) noexcept { m_isStunned = isStunned; }
    [[nodiscard]] bool IsStunned() const noexcept { return m_isStunned; }

    // Stan pojedynkow, celowania i indeksu combo
    void SetDuelMode(uint32_t mode) noexcept { m_duelMode = mode; }
    [[nodiscard]] uint32_t GetDuelMode() const noexcept { return m_duelMode; }
    void SetSkillTargetVID(uint32_t vid) noexcept { m_skillTargetVid = vid; }
    [[nodiscard]] uint32_t GetSkillTargetVID() const noexcept { return m_skillTargetVid; }
    void SetLastDmgActorVID(uint32_t vid) noexcept { m_lastDmgActorVid = vid; }
    [[nodiscard]] uint32_t GetLastDmgActorVID() const noexcept { return m_lastDmgActorVid; }
    void SetAdvActorVID(uint32_t vid) noexcept { m_advActorVid = vid; }
    [[nodiscard]] uint32_t GetAdvActorVID() const noexcept { return m_advActorVid; }
    void SetLastComboIndex(uint32_t idx) noexcept { m_lastComboIndex = idx; }
    [[nodiscard]] uint32_t GetLastComboIndex() const noexcept { return m_lastComboIndex; }

    // Rejestr kluczy PVP/GVG/DUEL (Thread-safe, hermetyczny)
    static void InsertPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void RemovePVPKey(uint32_t srcVid, uint32_t dstVid);
    [[nodiscard]] static bool HasPVPKey(uint32_t srcVid, uint32_t dstVid);

    static void InsertSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid);
    static void RemoveSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid);
    [[nodiscard]] static bool HasSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid);

    static void InsertPVPReadyKey(uint32_t srcVid, uint32_t dstVid);
    [[nodiscard]] static bool HasPVPReadyKey(uint32_t srcVid, uint32_t dstVid);

    static void InsertGVGKey(uint32_t srcGuildId, uint32_t dstGuildId);
    static void RemoveGVGKey(uint32_t srcGuildId, uint32_t dstGuildId);
    [[nodiscard]] static bool HasGVGKey(uint32_t srcGuildId, uint32_t dstGuildId);

    static void InsertDUELKey(uint32_t srcVid, uint32_t dstVid);
    [[nodiscard]] static bool HasDUELKey(uint32_t srcVid, uint32_t dstVid);

    static void ClearPVPKeys();
    static void ClearAllBattleKeys();

    [[nodiscard]] static uint32_t ComputeSymmetricKey(uint32_t srcVid, uint32_t dstVid) noexcept;

    void Clear();

private:
    std::list<DamageRecord> m_damageQueue;
    mutable std::shared_mutex m_damageMutex;

    CAffectFlagContainer m_affectFlags;
    uint32_t m_comboType{0};
    uint32_t m_attackSpeed{100};
    bool m_isDead{false};
    bool m_isStunned{false};

    uint32_t m_duelMode{0};
    uint32_t m_skillTargetVid{0};
    uint32_t m_lastDmgActorVid{0};
    uint32_t m_advActorVid{0};
    uint32_t m_lastComboIndex{0};

    static inline std::unordered_set<uint64_t> ms_pvpKeys;
    static inline std::unordered_set<uint32_t> ms_symmetricPvpKeys;
    static inline std::unordered_set<uint32_t> ms_pvpReadyKeys;
    static inline std::unordered_set<uint32_t> ms_gvgKeys;
    static inline std::unordered_set<uint32_t> ms_duelKeys;
    static inline std::shared_mutex ms_pvpMutex;

    static constexpr uint64_t MakePVPKey(uint32_t srcVid, uint32_t dstVid) noexcept {
        return (static_cast<uint64_t>(srcVid) << 32) | static_cast<uint64_t>(dstVid);
    }
};

} // namespace UserInterface::InstanceComponents
