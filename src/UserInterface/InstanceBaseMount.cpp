#include "StdAfx.h"
#include "InstanceBase.h"
#include "PythonBackground.h"
#include "PythonNonPlayer.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "AbstractPlayer.h"
#include "AbstractApplication.h"
#include "packet.h"

#include "EterLib/StateManager.h"
#include "GameLib/ItemManager.h"
#include "Core/EventBus.h"
#include "InstanceControllers/IInstanceMountHorseController.h"
#include "Client/Bridge/StranglerInstanceFacade.h"
#include "ECS/ECSWorldRegistry.h"

BOOL RIDE_HORSE_ENABLE = TRUE;
const float c_fDefaultHorseRotationSpeed = 300.0f;

//////////////////////////////////////////////////////////////////////////////////////
// CInstanceBase::SHORSE

CInstanceBase::SHORSE::SHORSE()
{
	__Initialize();
}

CInstanceBase::SHORSE::~SHORSE()
{
	assert(m_pkActor == NULL);
}

void CInstanceBase::SHORSE::__Initialize()
{
	m_isMounting = false;
	m_pkActor = NULL;
}

void CInstanceBase::SHORSE::SetAttackSpeed(UINT uAtkSpd)
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetAttackSpeed(uAtkSpd / 100.0f);
}

void CInstanceBase::SHORSE::SetMoveSpeed(UINT uMovSpd)
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetMoveSpeed(uMovSpd / 100.0f);
}

void CInstanceBase::SHORSE::Create(const TPixelPosition& c_rkPPos, UINT eRace, UINT eHitEffect)
{
	assert(NULL == m_pkActor && "CInstanceBase::SHORSE::Create - ALREADY MOUNT");

	m_pkActor = new CActorInstance;

	CActorInstance& rkActor = GetActorRef();
	rkActor.SetEventHandler(CActorInstance::IEventHandler::GetEmptyPtr());
	if (!rkActor.SetRace(eRace))
	{
		delete m_pkActor;
		m_pkActor = NULL;
		return;
	}

	rkActor.SetShape(0);
	rkActor.SetBattleHitEffect(eHitEffect);
	rkActor.SetAlphaValue(0.0f);
	rkActor.BlendAlphaValue(1.0f, 0.5f);
	rkActor.SetMoveSpeed(1.0f);
	rkActor.SetAttackSpeed(1.0f);
	rkActor.SetMotionMode(CRaceMotionData::MODE_GENERAL);
	rkActor.Stop();
	rkActor.RefreshActorInstance();

	rkActor.SetCurPixelPosition(c_rkPPos);

	m_isMounting = true;
}

void CInstanceBase::SHORSE::Destroy()
{
	if (m_pkActor)
	{
		m_pkActor->Destroy();
		delete m_pkActor;
	}

	__Initialize();
}

CActorInstance& CInstanceBase::SHORSE::GetActorRef()
{
	assert(NULL != m_pkActor && "CInstanceBase::SHORSE::GetActorRef");
	return *m_pkActor;
}

CActorInstance* CInstanceBase::SHORSE::GetActorPtr()
{
	return m_pkActor;
}

UINT CInstanceBase::SHORSE::GetLevel()
{
	if (m_pkActor)
	{
		DWORD mount = m_pkActor->GetRace();
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
		}

		// Mount expansion system: 20201 ~ 20212 and special event mounts
		{
			// Intermediate mounts: level 2 (attack enabled, skills disabled)
			if ((20205 <= mount && 20208 >= mount) ||
				(20214 == mount) || (20217 == mount) ||
				(20224 == mount) || (20229 == mount)
				)
				return 2;

			// Advanced mounts: level 3 (attack enabled, skills enabled)
			if ((20209 <= mount && 20212 >= mount) ||
				(20215 == mount) || (20218 == mount) ||
				(20220 == mount) || (20225 == mount) || (20230 == mount)
				)
				return 3;
		}
	}
	return 0;
}

bool CInstanceBase::SHORSE::IsNewMount()
{
	if (!m_pkActor)
		return false;
	DWORD mount = m_pkActor->GetRace();

	if ((20205 <= mount && 20208 >= mount) ||
		(20214 == mount) || (20217 == mount)
		)
		return true;

	if ((20209 <= mount && 20212 >= mount) ||
		(20215 == mount) || (20218 == mount) ||
		(20220 == mount)
		)
		return true;

	return false;
}

bool CInstanceBase::SHORSE::CanUseSkill()
{
	if (IsMounting())
		return 2 < GetLevel();

	return true;
}

bool CInstanceBase::SHORSE::CanAttack()
{
	if (IsMounting())
		if (GetLevel() <= 1)
			return false;

	return true;
}

bool CInstanceBase::SHORSE::IsMounting()
{
	return m_isMounting;
}

void CInstanceBase::SHORSE::Deform()
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.INSTANCEBASE_Deform();
}

void CInstanceBase::SHORSE::Render()
{
	if (!IsMounting())
		return;

	CActorInstance& rkActor = GetActorRef();
	rkActor.Render();
}

//////////////////////////////////////////////////////////////////////////////////////
// CInstanceBase Mount / Horse Operations

void CInstanceBase::__AttachHorseSaddle()
{
	if (!IsMountingHorse())
		return;
	m_kHorse.m_pkActor->AttachModelInstance(CRaceData::PART_MAIN, "saddle", m_GraphicThingInstance, CRaceData::PART_MAIN);
}

void CInstanceBase::__DetachHorseSaddle()
{
	if (!IsMountingHorse())
		return;
	m_kHorse.m_pkActor->DetachModelInstance(CRaceData::PART_MAIN, m_GraphicThingInstance, CRaceData::PART_MAIN);
}

BOOL CInstanceBase::IsNewMount()
{
	return m_kHorse.IsNewMount();
}

BOOL CInstanceBase::IsMountingHorse()
{
	return m_kHorse.IsMounting();
}

void CInstanceBase::MountHorse(UINT eRace)
{
	m_kHorse.Destroy();
	m_kHorse.Create(m_GraphicThingInstance.NEW_GetCurPixelPositionRef(), eRace, ms_adwCRCAffectEffect[EFFECT_HIT]);

	SetMotionMode(CRaceMotionData::MODE_HORSE);
	SetRotationSpeed(c_fDefaultHorseRotationSpeed);

	m_GraphicThingInstance.MountHorse(m_kHorse.GetActorPtr());
	m_GraphicThingInstance.Stop();
	m_GraphicThingInstance.RefreshActorInstance();

	const DWORD m_dwVID = GetVirtualID();
	UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::MountStateChangedEvent(m_dwVID, eRace, 1));
}

void CInstanceBase::DismountHorse()
{
	m_kHorse.Destroy();

	const DWORD m_dwVID = GetVirtualID();
	UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::MountStateChangedEvent(m_dwVID, 0, 0));
}

BOOL CInstanceBase::CanAttackHorseLevel()
{
	if (!IsMountingHorse())
		return FALSE;

	return m_kHorse.CanAttack();
}

UINT CInstanceBase::GetHorseLevel()
{
	if (!IsMountingHorse())
		return 0;

	return m_kHorse.GetLevel();
}

void CInstanceBase::UpdateHorseMotion()
{
	if (IsMountingHorse())
	{
		if (m_kHorse.m_pkActor)
			m_kHorse.m_pkActor->HORSE_MotionProcess(FALSE);
	}
}

void CInstanceBase::ProcessHorseDust()
{
	if (IsMountingHorse())
	{
		__AttachEffect(EFFECT_HORSE_DUST);
	}
}

int CInstanceBase::GetHorseMotionMode(BYTE byWeaponSubType)
{
	switch (byWeaponSubType)
	{
		case CItemData::WEAPON_SWORD:
			return CRaceMotionData::MODE_HORSE_ONEHAND_SWORD;

		case CItemData::WEAPON_TWO_HANDED:
			return CRaceMotionData::MODE_HORSE_TWOHAND_SWORD; // Only Warrior

		case CItemData::WEAPON_DAGGER:
			return CRaceMotionData::MODE_HORSE_DUALHAND_SWORD; // Only Assassin

		case CItemData::WEAPON_FAN:
			return CRaceMotionData::MODE_HORSE_FAN; // Only Shaman

		case CItemData::WEAPON_BELL:
			return CRaceMotionData::MODE_HORSE_BELL; // Only Shaman

		case CItemData::WEAPON_BOW:
			return CRaceMotionData::MODE_HORSE_BOW; // Only Shaman

		default:
			return CRaceMotionData::MODE_HORSE;
	}
}
