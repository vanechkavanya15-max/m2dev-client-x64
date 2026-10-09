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
// CInstanceBase Mount / Horse Operations (Kompozycyjna fasada do InstanceMountComponent)

void CInstanceBase::__AttachHorseSaddle()
{
	m_mountComponent.AttachSaddle(m_GraphicThingInstance);
}

void CInstanceBase::__DetachHorseSaddle()
{
	m_mountComponent.DetachSaddle(m_GraphicThingInstance);
}

BOOL CInstanceBase::IsNewMount()
{
	return m_mountComponent.IsNewMount();
}

BOOL CInstanceBase::IsMountingHorse()
{
	return m_mountComponent.IsMounting();
}

void CInstanceBase::MountHorse(UINT eRace)
{
	m_mountComponent.Mount(
		m_GraphicThingInstance,
		m_GraphicThingInstance.NEW_GetCurPixelPositionRef(),
		eRace,
		ms_adwCRCAffectEffect[EFFECT_HIT]
	);

	SetMotionMode(CRaceMotionData::MODE_HORSE);
	SetRotationSpeed(c_fDefaultHorseRotationSpeed);

	const DWORD dwVID = GetVirtualID();
	UserInterface::Core::EventBus::GetInstance().Publish(
		UserInterface::Core::MountStateChangedEvent(dwVID, eRace, 1)
	);
}

void CInstanceBase::DismountHorse()
{
	m_mountComponent.Dismount(m_GraphicThingInstance);

	const DWORD dwVID = GetVirtualID();
	UserInterface::Core::EventBus::GetInstance().Publish(
		UserInterface::Core::MountStateChangedEvent(dwVID, 0, 0)
	);
}

BOOL CInstanceBase::CanAttackHorseLevel()
{
	if (!IsMountingHorse())
		return FALSE;

	return m_mountComponent.CanAttack();
}

UINT CInstanceBase::GetHorseLevel()
{
	if (!IsMountingHorse())
		return 0;

	return m_mountComponent.GetLevel();
}

void CInstanceBase::UpdateHorseMotion()
{
	m_mountComponent.UpdateMotion();
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
	return UserInterface::InstanceComponents::InstanceMountComponent::GetHorseMotionMode(byWeaponSubType);
}
