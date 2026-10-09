#include "StdAfx.h"
#include "InstanceMountComponent.h"
#include <cassert>
#include <utility>

namespace UserInterface::InstanceComponents {

InstanceMountComponent::InstanceMountComponent()
{
    Initialize();
}

InstanceMountComponent::~InstanceMountComponent()
{
    Destroy();
}

InstanceMountComponent::InstanceMountComponent(InstanceMountComponent&& other) noexcept
    : m_isMounting(other.m_isMounting)
    , m_mountRace(other.m_mountRace)
    , m_pkActor(other.m_pkActor)
    , m_stateCallback(std::move(other.m_stateCallback))
{
    other.m_isMounting = false;
    other.m_mountRace = 0;
    other.m_pkActor = nullptr;
}

InstanceMountComponent& InstanceMountComponent::operator=(InstanceMountComponent&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_isMounting = other.m_isMounting;
        m_mountRace = other.m_mountRace;
        m_pkActor = other.m_pkActor;
        m_stateCallback = std::move(other.m_stateCallback);

        other.m_isMounting = false;
        other.m_mountRace = 0;
        other.m_pkActor = nullptr;
    }
    return *this;
}

void InstanceMountComponent::Initialize()
{
    m_isMounting = false;
    m_mountRace = 0;
    m_pkActor = nullptr;
}

void InstanceMountComponent::Create(const TPixelPosition& pos, uint32_t mountRace, uint32_t hitEffect)
{
    Destroy();

    m_pkActor = new CActorInstance;
    m_pkActor->SetEventHandler(CActorInstance::IEventHandler::GetEmptyPtr());

    if (!m_pkActor->SetRace(mountRace))
    {
        delete m_pkActor;
        m_pkActor = nullptr;
        return;
    }

    m_pkActor->SetShape(0);
    m_pkActor->SetBattleHitEffect(hitEffect);
    m_pkActor->SetAlphaValue(0.0f);
    m_pkActor->BlendAlphaValue(1.0f, 0.5f);
    m_pkActor->SetMoveSpeed(1.0f);
    m_pkActor->SetAttackSpeed(1.0f);
    m_pkActor->SetMotionMode(CRaceMotionData::MODE_GENERAL);
    m_pkActor->Stop();
    m_pkActor->RefreshActorInstance();
    m_pkActor->SetCurPixelPosition(pos);

    m_isMounting = true;
    m_mountRace = mountRace;
}

bool InstanceMountComponent::Mount(CActorInstance& riderActor, const TPixelPosition& pos, uint32_t mountRace, uint32_t hitEffect)
{
    Create(pos, mountRace, hitEffect);
    if (!m_pkActor)
    {
        return false;
    }

    riderActor.MountHorse(m_pkActor);
    riderActor.Stop();
    riderActor.RefreshActorInstance();

    if (m_stateCallback)
    {
        m_stateCallback(mountRace, 1);
    }

    return true;
}

void InstanceMountComponent::Dismount(CActorInstance& riderActor)
{
    uint32_t oldRace = m_mountRace;
    Destroy();
    riderActor.MountHorse(nullptr);

    if (m_stateCallback)
    {
        m_stateCallback(oldRace, 0);
    }
}

void InstanceMountComponent::Destroy()
{
    if (m_pkActor)
    {
        m_pkActor->Destroy();
        delete m_pkActor;
        m_pkActor = nullptr;
    }
    Initialize();
}

void InstanceMountComponent::Clear()
{
    Destroy();
    m_stateCallback = nullptr;
}

void InstanceMountComponent::Deform()
{
    if (m_isMounting && m_pkActor)
    {
        m_pkActor->INSTANCEBASE_Deform();
    }
}

void InstanceMountComponent::Render()
{
    if (m_isMounting && m_pkActor)
    {
        m_pkActor->Render();
    }
}

void InstanceMountComponent::UpdateMotion()
{
    if (m_isMounting && m_pkActor)
    {
        m_pkActor->HORSE_MotionProcess(FALSE);
    }
}

void InstanceMountComponent::AttachSaddle(CActorInstance& riderActor)
{
    if (!m_isMounting || !m_pkActor)
        return;

    m_pkActor->AttachModelInstance(CRaceData::PART_MAIN, "saddle", riderActor, CRaceData::PART_MAIN);
}

void InstanceMountComponent::DetachSaddle(CActorInstance& riderActor)
{
    if (!m_isMounting || !m_pkActor)
        return;

    m_pkActor->DetachModelInstance(CRaceData::PART_MAIN, riderActor, CRaceData::PART_MAIN);
}

void InstanceMountComponent::SetAttackSpeed(uint32_t atkSpd)
{
    if (m_isMounting && m_pkActor)
    {
        m_pkActor->SetAttackSpeed(atkSpd / 100.0f);
    }
}

void InstanceMountComponent::SetMoveSpeed(uint32_t movSpd)
{
    if (m_isMounting && m_pkActor)
    {
        m_pkActor->SetMoveSpeed(movSpd / 100.0f);
    }
}

uint32_t InstanceMountComponent::GetLevel() const noexcept
{
    if (!m_isMounting)
        return 0;

    return CalculateMountLevel(m_mountRace);
}

bool InstanceMountComponent::IsNewMount() const noexcept
{
    if (!m_isMounting)
        return false;

    return IsNewMountRace(m_mountRace);
}

bool InstanceMountComponent::CanAttack() const noexcept
{
    if (m_isMounting)
    {
        return GetLevel() > 1;
    }
    return true;
}

bool InstanceMountComponent::CanUseSkill() const noexcept
{
    if (m_isMounting)
    {
        return GetLevel() > 2;
    }
    return true;
}

CActorInstance& InstanceMountComponent::GetActorRef()
{
    assert(m_pkActor != nullptr && "InstanceMountComponent::GetActorRef called on null actor");
    return *m_pkActor;
}

const CActorInstance& InstanceMountComponent::GetActorRef() const
{
    assert(m_pkActor != nullptr && "InstanceMountComponent::GetActorRef called on null actor");
    return *m_pkActor;
}

uint32_t InstanceMountComponent::CalculateMountLevel(uint32_t mount) noexcept
{
    switch (mount)
    {
        case 20101:
        case 20102:
        case 20103:
            return 1;
        case 20104:
        case 20105:
        case 20106:
            return 2;
        case 20107:
        case 20108:
        case 20109:
        case 20110:
        case 20111:
        case 20112:
        case 20113:
        case 20114:
        case 20115:
        case 20116:
        case 20117:
        case 20118:
        case 20120:
        case 20121:
        case 20122:
        case 20123:
        case 20124:
        case 20125:
            return 3;
        case 20119:
        case 20219:
        case 20220:
        case 20221:
        case 20222:
            return 2;
        default:
            break;
    }

    if ((20205 <= mount && mount <= 20208) ||
        (20214 == mount) || (20217 == mount) ||
        (20224 == mount) || (20229 == mount))
    {
        return 2;
    }

    if ((20209 <= mount && mount <= 20212) ||
        (20215 == mount) || (20218 == mount) ||
        (20220 == mount) || (20225 == mount) || (20230 == mount))
    {
        return 3;
    }

    return 0;
}

bool InstanceMountComponent::IsNewMountRace(uint32_t mount) noexcept
{
    if ((20205 <= mount && mount <= 20208) ||
        (20214 == mount) || (20217 == mount))
    {
        return true;
    }

    if ((20209 <= mount && mount <= 20212) ||
        (20215 == mount) || (20218 == mount) ||
        (20220 == mount))
    {
        return true;
    }

    return false;
}

int InstanceMountComponent::GetHorseMotionMode(uint8_t weaponSubType) noexcept
{
    switch (weaponSubType)
    {
        case CItemData::WEAPON_SWORD:
            return CRaceMotionData::MODE_HORSE_ONEHAND_SWORD;
        case CItemData::WEAPON_TWO_HANDED:
            return CRaceMotionData::MODE_HORSE_TWOHAND_SWORD;
        case CItemData::WEAPON_DAGGER:
            return CRaceMotionData::MODE_HORSE_DUALHAND_SWORD;
        case CItemData::WEAPON_FAN:
            return CRaceMotionData::MODE_HORSE_FAN;
        case CItemData::WEAPON_BELL:
            return CRaceMotionData::MODE_HORSE_BELL;
        case CItemData::WEAPON_BOW:
            return CRaceMotionData::MODE_HORSE_BOW;
        default:
            return CRaceMotionData::MODE_HORSE;
    }
}

} // namespace UserInterface::InstanceComponents
