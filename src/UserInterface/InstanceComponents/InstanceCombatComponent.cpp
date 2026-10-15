#include "StdAfx.h"
#include "InstanceCombatComponent.h"
#include <algorithm>

namespace UserInterface::InstanceComponents {

void InstanceCombatComponent::AddDamage(uint32_t damage, uint8_t flag, bool isSelf, bool isTarget)
{
    std::unique_lock lock(m_damageMutex);
    m_damageQueue.push_back(DamageRecord{
        .damage = damage,
        .flag = flag,
        .isSelf = isSelf,
        .isTarget = isTarget
    });
}

std::list<InstanceCombatComponent::DamageRecord> InstanceCombatComponent::PopAllDamages()
{
    std::unique_lock lock(m_damageMutex);
    std::list<DamageRecord> result;
    result.swap(m_damageQueue);
    return result;
}

bool InstanceCombatComponent::PopDamage(DamageRecord& outRecord)
{
    std::unique_lock lock(m_damageMutex);
    if (m_damageQueue.empty())
    {
        return false;
    }
    outRecord = m_damageQueue.front();
    m_damageQueue.pop_front();
    return true;
}

size_t InstanceCombatComponent::GetPendingDamageCount() const noexcept
{
    std::shared_lock lock(m_damageMutex);
    return m_damageQueue.size();
}

bool InstanceCombatComponent::HasDamages() const noexcept
{
    std::shared_lock lock(m_damageMutex);
    return !m_damageQueue.empty();
}

void InstanceCombatComponent::ClearDamages()
{
    std::unique_lock lock(m_damageMutex);
    m_damageQueue.clear();
}

void InstanceCombatComponent::InsertPVPKey(uint32_t srcVid, uint32_t dstVid)
{
    std::unique_lock lock(ms_pvpMutex);
    ms_pvpKeys.insert(MakePVPKey(srcVid, dstVid));
}

void InstanceCombatComponent::RemovePVPKey(uint32_t srcVid, uint32_t dstVid)
{
    std::unique_lock lock(ms_pvpMutex);
    ms_pvpKeys.erase(MakePVPKey(srcVid, dstVid));
}

bool InstanceCombatComponent::HasPVPKey(uint32_t srcVid, uint32_t dstVid)
{
    std::shared_lock lock(ms_pvpMutex);
    return ms_pvpKeys.contains(MakePVPKey(srcVid, dstVid));
}

void InstanceCombatComponent::ClearPVPKeys()
{
    std::unique_lock lock(ms_pvpMutex);
    ms_pvpKeys.clear();
}

uint32_t InstanceCombatComponent::ComputeSymmetricKey(uint32_t srcVid, uint32_t dstVid) noexcept
{
    if (srcVid > dstVid)
    {
        std::swap(srcVid, dstVid);
    }

    uint32_t awSrc[2] = { srcVid, dstVid };
    const uint8_t* s = reinterpret_cast<const uint8_t*>(awSrc);
    const uint8_t* end = s + sizeof(awSrc);
    uint32_t h = 0;

    while (s < end)
    {
        h *= 16777619;
        h ^= static_cast<uint32_t>(*s++);
    }

    return h;
}

void InstanceCombatComponent::InsertSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::unique_lock lock(ms_pvpMutex);
    ms_symmetricPvpKeys.insert(key);
}

void InstanceCombatComponent::RemoveSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::unique_lock lock(ms_pvpMutex);
    ms_symmetricPvpKeys.erase(key);
}

bool InstanceCombatComponent::HasSymmetricPVPKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::shared_lock lock(ms_pvpMutex);
    return ms_symmetricPvpKeys.contains(key);
}

void InstanceCombatComponent::InsertPVPReadyKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::unique_lock lock(ms_pvpMutex);
    ms_pvpReadyKeys.insert(key);
}

bool InstanceCombatComponent::HasPVPReadyKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::shared_lock lock(ms_pvpMutex);
    return ms_pvpReadyKeys.contains(key);
}

void InstanceCombatComponent::InsertGVGKey(uint32_t srcGuildId, uint32_t dstGuildId)
{
    uint32_t key = ComputeSymmetricKey(srcGuildId, dstGuildId);
    std::unique_lock lock(ms_pvpMutex);
    ms_gvgKeys.insert(key);
}

void InstanceCombatComponent::RemoveGVGKey(uint32_t srcGuildId, uint32_t dstGuildId)
{
    uint32_t key = ComputeSymmetricKey(srcGuildId, dstGuildId);
    std::unique_lock lock(ms_pvpMutex);
    ms_gvgKeys.erase(key);
}

bool InstanceCombatComponent::HasGVGKey(uint32_t srcGuildId, uint32_t dstGuildId)
{
    uint32_t key = ComputeSymmetricKey(srcGuildId, dstGuildId);
    std::shared_lock lock(ms_pvpMutex);
    return ms_gvgKeys.contains(key);
}

void InstanceCombatComponent::InsertDUELKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::unique_lock lock(ms_pvpMutex);
    ms_duelKeys.insert(key);
}

bool InstanceCombatComponent::HasDUELKey(uint32_t srcVid, uint32_t dstVid)
{
    uint32_t key = ComputeSymmetricKey(srcVid, dstVid);
    std::shared_lock lock(ms_pvpMutex);
    return ms_duelKeys.contains(key);
}

void InstanceCombatComponent::ClearAllBattleKeys()
{
    std::unique_lock lock(ms_pvpMutex);
    ms_pvpKeys.clear();
    ms_symmetricPvpKeys.clear();
    ms_pvpReadyKeys.clear();
    ms_gvgKeys.clear();
    ms_duelKeys.clear();
}

void InstanceCombatComponent::Clear()
{
    ClearDamages();
    m_affectFlags.Clear();
    m_comboType = 0;
    m_attackSpeed = 100;
    m_isDead = false;
    m_isStunned = false;

    m_duelMode = 0;
    m_skillTargetVid = 0;
    m_lastDmgActorVid = 0;
    m_advActorVid = 0;
    m_lastComboIndex = 0;
}

} // namespace UserInterface::InstanceComponents
