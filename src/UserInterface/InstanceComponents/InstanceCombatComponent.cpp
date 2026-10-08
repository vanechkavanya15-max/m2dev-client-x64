#include "StdAfx.h"
#include "InstanceCombatComponent.h"

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

size_t InstanceCombatComponent::GetPendingDamageCount() const noexcept
{
    std::shared_lock lock(m_damageMutex);
    return m_damageQueue.size();
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

void InstanceCombatComponent::Clear()
{
    ClearDamages();
    m_affectFlags.Clear();
    m_comboType = 0;
    m_attackSpeed = 100;
    m_isDead = false;
    m_isStunned = false;
}

} // namespace UserInterface::InstanceComponents
