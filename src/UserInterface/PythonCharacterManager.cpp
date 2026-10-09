#include "stdafx.h"
#include "pythoncharactermanager.h"
#include "PythonBackground.h"
#include "PythonNonPlayer.h"
#include "AbstractPlayer.h"
#include "packet.h"

#include "EterLib/Camera.h"
#include "ECS/ECSWorldRegistry.h"
#include "Core/EventBus.h"

using Client::World::EntityVid;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Frame Process

int CHAR_STAGE_VIEW_BOUND = 200 * 100;

void CPythonCharacterManager::AdjustCollisionWithOtherObjects(CActorInstance* targetActor)
{
	if (!targetActor || !targetActor->IsPC())
		return;

	for (const auto& [vid, actor] : m_aliveActorsMap)
	{
		if (!actor)
			continue;

		CActorInstance* graphicActor = actor->GetGraphicThingInstancePtr();
		if (!graphicActor || graphicActor == targetActor)
			continue;

		if (graphicActor->IsPC() || graphicActor->IsNPC() || graphicActor->IsEnemy())
			continue;

		if (targetActor->TestPhysicsBlendingCollision(*graphicActor))
		{
			TPixelPosition curPos;
			targetActor->GetPixelPosition(&curPos);
			targetActor->SetBlendingPosition(curPos);
			break;
		}
	}
}

void CPythonCharacterManager::EnableSortRendering(bool isEnable)
{
}

void CPythonCharacterManager::InsertPVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	CInstanceBase::InsertPVPKey(dwVIDSrc, dwVIDDst);

	CInstanceBase* srcActor = GetInstancePtr(dwVIDSrc);
	if (srcActor)
		srcActor->RefreshTextTail();

	CInstanceBase* dstActor = GetInstancePtr(dwVIDDst);
	if (dstActor)
		dstActor->RefreshTextTail();
}

void CPythonCharacterManager::RemovePVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	CInstanceBase::RemovePVPKey(dwVIDSrc, dwVIDDst);

	CInstanceBase* srcActor = GetInstancePtr(dwVIDSrc);
	if (srcActor)
		srcActor->RefreshTextTail();

	CInstanceBase* dstActor = GetInstancePtr(dwVIDDst);
	if (dstActor)
		dstActor->RefreshTextTail();
}

void CPythonCharacterManager::ChangeGVG(DWORD dwSrcGuildID, DWORD dwDstGuildID)
{
	for (auto& pair : m_aliveActorsMap)
	{
		CInstanceBase* pInstance = pair.second;
		DWORD dwInstanceGuildID = pInstance->GetGuildID();
		if (dwSrcGuildID == dwInstanceGuildID || dwDstGuildID == dwInstanceGuildID)
		{
			pInstance->RefreshTextTail();
		}
	}
}

void CPythonCharacterManager::ClearMainInstance()
{
	m_mainActor = NULL;
	m_actorRegistry.SetMainActorVid(EntityVid(0));
	m_actorProviderAdapter.SetMainActorVID(0);
}

bool CPythonCharacterManager::SetMainInstance(DWORD dwVID)
{
	m_mainActor = GetInstancePtr(dwVID);
	if (!m_mainActor)
		return false;

	m_actorRegistry.SetMainActorVid(EntityVid(dwVID));
	m_actorProviderAdapter.SetMainActorVID(dwVID);
	return true;
}

CInstanceBase* CPythonCharacterManager::GetMainActorPtr()
{
	return m_mainActor;
}

EntityVid CPythonCharacterManager::GetMainActorVid() const noexcept
{
	return m_actorRegistry.GetMainActorVid();
}

std::optional<Client::Actor::EntityHandle> CPythonCharacterManager::GetMainActorHandle() const noexcept
{
	if (m_mainActor)
	{
		return m_mainActor->GetGenerationalHandle();
	}
	return std::nullopt;
}

std::optional<Client::Actor::EntityHandle> CPythonCharacterManager::GetActorHandle(DWORD vid) const noexcept
{
	auto it = m_aliveActorsMap.find(vid);
	if (it != m_aliveActorsMap.end() && it->second)
	{
		return it->second->GetGenerationalHandle();
	}
	return std::nullopt;
}

std::optional<Client::Actor::EntityHandle> CPythonCharacterManager::GetActorHandle(EntityVid vid) const noexcept
{
	return GetActorHandle(vid.get());
}

bool CPythonCharacterManager::IsActorAlive(EntityVid vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsAliveVID(vid.get());
}

bool CPythonCharacterManager::IsActorAlive(DWORD vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsAliveVID(vid);
}

bool CPythonCharacterManager::IsActorDead(EntityVid vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsDeadVID(vid.get());
}

bool CPythonCharacterManager::IsActorDead(DWORD vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsDeadVID(vid);
}

bool CPythonCharacterManager::HasActor(EntityVid vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsRegisteredVID(vid.get());
}

bool CPythonCharacterManager::HasActor(DWORD vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->IsRegisteredVID(vid);
}

CInstanceBase* CPythonCharacterManager::GetActor(EntityVid vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->GetInstancePtr(vid.get());
}

CInstanceBase* CPythonCharacterManager::GetActor(DWORD vid) const noexcept
{
	return const_cast<CPythonCharacterManager*>(this)->GetInstancePtr(vid);
}

std::optional<EntityVid> CPythonCharacterManager::GetPickedActorVid() const noexcept
{
	if (m_pickedActor)
	{
		return EntityVid(m_pickedActor->GetVirtualID());
	}
	return std::nullopt;
}

std::optional<EntityVid> CPythonCharacterManager::GetSelectedActorVid() const noexcept
{
	if (m_boundActor)
	{
		return EntityVid(m_boundActor->GetVirtualID());
	}
	return std::nullopt;
}

bool CPythonCharacterManager::SetMainActor(EntityVid vid) noexcept
{
	return SetMainInstance(vid.get());
}

size_t CPythonCharacterManager::GetActorCount() const noexcept
{
	return m_aliveActorsMap.size();
}

void CPythonCharacterManager::GetInfo(std::string* pstInfo)
{
	pstInfo->append("Actor: ");
	CInstanceBase::GetInfo(pstInfo);

	char szInfo[256];
	sprintf(szInfo, "Container - Live %zd, Dead %zd, Grid %zd",
		m_aliveActorsMap.size(),
		m_sceneMgr.GetDeadCount(),
		m_spatialGrid.Count());
	pstInfo->append(szInfo);
}

bool CPythonCharacterManager::IsCacheMode()
{
	static bool s_isOldCacheMode = false;

	bool isCacheMode = s_isOldCacheMode;
	if (s_isOldCacheMode)
	{
		if (m_aliveActorsMap.size() < 30)
			isCacheMode = false;
	}
	else
	{
		if (m_aliveActorsMap.size() > 40)
			isCacheMode = true;
	}
	s_isOldCacheMode = isCacheMode;

	return isCacheMode;
}

void CPythonCharacterManager::Update()
{
	CInstanceBase::ResetPerformanceCounter();

	TCharacterInstanceMap::iterator i = m_aliveActorsMap.begin();
	while (m_aliveActorsMap.end() != i)
	{
		TCharacterInstanceMap::iterator c = i++;

		CInstanceBase* pkInstEach = c->second;
		if (!pkInstEach)
			continue;

		pkInstEach->Update();

		// Synchronize spatial grid, world actor registry, and ECS transform table
		if (pkInstEach->GetGraphicThingInstanceRef().IsMovement())
		{
			TPixelPosition curPos;
			pkInstEach->NEW_GetPixelPosition(&curPos);
			CentralUpdateActorPosition(
				pkInstEach->GetVirtualID(),
				curPos.x,
				curPos.y,
				curPos.z,
				pkInstEach->GetRotation()
			);
		}
	}

	UpdateTransform();
	m_sceneMgr.UpdateDeleting([this](DWORD vid) {
		m_actorRegistry.UnregisterActor(EntityVid(vid));
	});
	__NEW_Pick();
}

void CPythonCharacterManager::ShowPointEffect(DWORD ePoint, DWORD dwVID)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetMainActorPtr() : GetInstancePtr(dwVID);
	if (!pkInstSel)
		return;

	switch (ePoint)
	{
		case POINT_LEVEL:
			pkInstSel->LevelUp();
			break;
		case POINT_LEVEL_STEP:
			pkInstSel->SkillUp();
			break;
	}
}

bool CPythonCharacterManager::RegisterPointEffect(DWORD ePoint, const char* c_szFileName)
{
	if (ePoint >= POINT_MAX_NUM)
		return false;

	CEffectManager& rkEftMgr = CEffectManager::Instance();
	rkEftMgr.RegisterEffect2(c_szFileName, &m_adwPointEffect[ePoint]);

	return true;
}

void CPythonCharacterManager::UpdateTransform()
{
	CInstanceBase* pMainInstance = GetMainActorPtr();
	if (pMainInstance)
	{
		CPythonBackground& rkBG = CPythonBackground::Instance();
		for (auto& pair : m_aliveActorsMap)
		{
			CInstanceBase* pSrcInstance = pair.second;
			pSrcInstance->CheckAdvancing();

			if (pSrcInstance->IsPushing())
				rkBG.CheckAdvancing(pSrcInstance);
		}

		rkBG.CheckAdvancing(m_mainActor);
	}

	for (auto& pair : m_aliveActorsMap)
	{
		pair.second->Transform();
	}
}

void CPythonCharacterManager::UpdateDeleting()
{
	m_sceneMgr.UpdateDeleting([this](DWORD vid) {
		m_actorRegistry.UnregisterActor(EntityVid(vid));
	});
}

void CPythonCharacterManager::Deform()
{
	m_sceneMgr.Deform();
}

bool CPythonCharacterManager::GetPickedActorID(DWORD* pdwPickedActorID)
{
	if (!m_pickedActor)
		return false;
		
	*pdwPickedActorID = m_pickedActor->GetVirtualID();
	return true;
}

CInstanceBase* CPythonCharacterManager::GetPickedActorPtr()
{
	if (m_pickedActor && !m_subsystemActorRegistry.ContainsActor(m_pickedActor->GetVirtualID()))
	{
		m_pickedActor = nullptr;
	}
	return m_pickedActor;
}

D3DXVECTOR2& CPythonCharacterManager::GetPickedActorScreenPos()
{
	return m_pickedActorScreenPos;
}

bool CPythonCharacterManager::IsRegisteredVID(DWORD dwVID)
{
	return m_subsystemActorRegistry.ContainsActor(dwVID) || m_sceneMgr.IsDead(dwVID) || m_actorRegistry.HasActor(EntityVid(dwVID));
}

bool CPythonCharacterManager::IsAliveVID(DWORD dwVID)
{
	return m_subsystemActorRegistry.ContainsActor(dwVID);
}

bool CPythonCharacterManager::IsDeadVID(DWORD dwVID)
{
	return m_sceneMgr.IsDead(dwVID) || m_actorRegistry.IsDead(EntityVid(dwVID));
}

void CPythonCharacterManager::__RenderSortedAliveActorList()
{
	CCamera* pCamera = CCameraManager::instance().GetCurrentCamera();
	if (pCamera)
		m_sceneMgr.SortAliveInstances(pCamera->GetEye());
}

void CPythonCharacterManager::__RenderSortedDeadActorList()
{
	CCamera* pCamera = CCameraManager::instance().GetCurrentCamera();
	if (pCamera)
		m_sceneMgr.SortDeadInstances(pCamera->GetEye());
}

void CPythonCharacterManager::Render()
{
	m_sceneMgr.Render();

	CInstanceBase* pkPickedInst = GetPickedActorPtr();
	if (pkPickedInst)
	{
		const D3DXVECTOR3& c_rv3Position = pkPickedInst->GetGraphicThingInstanceRef().GetPosition();
		CPythonGraphic::Instance().ProjectPosition(c_rv3Position.x, c_rv3Position.y, c_rv3Position.z, &m_pickedActorScreenPos.x, &m_pickedActorScreenPos.y);
	}
}

void CPythonCharacterManager::RenderShadowMainInstance()
{
	m_sceneMgr.RenderShadowMainInstance(GetMainActorPtr());
}

void CPythonCharacterManager::RenderShadowAllInstances()
{
	m_sceneMgr.RenderShadowAllInstances();
}

void CPythonCharacterManager::RenderCollision()
{
	m_sceneMgr.RenderCollision();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Centralized Single-Source-of-Truth Lifecycle Operations

bool CPythonCharacterManager::CentralRegisterActor(DWORD dwVID, CInstanceBase* pInst, const CInstanceBase::SCreateData* pCreateData)
{
	if (dwVID == 0 || !pInst)
		return false;

	if (m_subsystemActorRegistry.ContainsActor(dwVID))
		return false;

	// 1. Rejestracja w glownym rejestrze domenowym instancji postaci (Single Source of Truth)
	if (!m_subsystemActorRegistry.RegisterActor(dwVID, pInst))
		return false;

	// 2. Synchronizacja z mapa zywych aktorow (zgodnosc wsteczna i iteratory)
	m_aliveActorsMap[dwVID] = pInst;

	// 3. Rejestracja w menedzerze sceny graficznej
	m_sceneMgr.AddAliveInstance(pInst);

	// 4. Rejestracja w rejestrze swiata Client::World::ActorRegistry
	float fPosX = pCreateData ? static_cast<float>(pCreateData->m_lPosX) : 0.0f;
	float fPosY = pCreateData ? static_cast<float>(pCreateData->m_lPosY) : 0.0f;
	float fPosZ = 0.0f;
	float fRot  = pCreateData ? pCreateData->m_fRot : 0.0f;
	DWORD dwRace = pCreateData ? pCreateData->m_dwRace : 0;
	std::string stName = (pCreateData && !pCreateData->m_stName.empty()) ? pCreateData->m_stName : "";

	Client::World::ActorRecord record{
		.vid = EntityVid(dwVID),
		.race = dwRace,
		.type = 0,
		.x = fPosX,
		.y = fPosY,
		.z = fPosZ,
		.rotation = fRot,
		.name = std::move(stName),
		.guildId = 0,
		.empire = 0,
		.isDead = false
	};
	m_actorRegistry.RegisterActor(record);

	// 5. Rejestracja w siatce przestrzennej SpatialHashGrid
	m_spatialGrid.Insert(EntityVid(dwVID), fPosX, fPosY);

	// 6. Rejestracja w tabelach SoA C++23 ECSWorldRegistry
	(void)UserInterface::ECS::ECSWorldRegistry::GetInstance().RegisterEntity(
		EterBase::EntityId(dwVID),
		fPosX,
		fPosY,
		fPosZ,
		fRot,
		0.0f
	);

	// Obsluga glownego gracza
	if (pCreateData && pCreateData->m_isMain)
	{
		SelectInstance(dwVID);
		m_actorRegistry.SetMainActorVid(EntityVid(dwVID));
		m_actorProviderAdapter.SetMainActorVID(dwVID);
		m_mainActor = pInst;
	}

	return true;
}

bool CPythonCharacterManager::CentralUnregisterActor(DWORD dwVID, bool bFadeOut)
{
	if (dwVID == 0)
		return false;

	CInstanceBase* pkInst = m_subsystemActorRegistry.FindActor(dwVID);

	// Jezeli aktor nie znajduje sie wsrod zywych instancji, sprawdz czy nie dogasa w m_deadInstances
	if (!pkInst)
	{
		if (!bFadeOut)
		{
			for (CInstanceBase* pkDeadInst : m_sceneMgr.GetDeadInstances())
			{
				if (pkDeadInst && pkDeadInst->GetVirtualID() == dwVID)
				{
					if (pkDeadInst == m_boundActor)
						m_boundActor = nullptr;
					if (pkDeadInst == m_mainActor)
					{
						m_mainActor = nullptr;
						m_actorRegistry.SetMainActorVid(EntityVid(0));
						m_actorProviderAdapter.SetMainActorVID(0);
					}
					if (pkDeadInst == m_pickedActor)
					{
						pkDeadInst->OnUnselected();
						m_pickedActor = nullptr;
					}
					std::erase(m_pickedInstances, pkDeadInst);

					m_sceneMgr.RemoveDeadInstance(pkDeadInst);
					m_actorRegistry.UnregisterActor(EntityVid(dwVID));
					CInstanceBase::Delete(pkDeadInst);
					return true;
				}
			}
		}
		return false;
	}

	// 1. Oczyszczenie referencji wyboru (zapobieganie wiszacym wskaznikom - dangling pointers)
	if (pkInst == m_boundActor)
		m_boundActor = nullptr;

	if (pkInst == m_mainActor)
	{
		m_mainActor = nullptr;
		m_actorRegistry.SetMainActorVid(EntityVid(0));
		m_actorProviderAdapter.SetMainActorVID(0);
	}

	if (pkInst == m_pickedActor)
	{
		pkInst->OnUnselected();
		m_pickedActor = nullptr;
	}

	std::erase(m_pickedInstances, pkInst);

	// 2. Wyrejestrowanie z ECS, SpatialHashGrid oraz zywego rejestru domenowego
	(void)UserInterface::ECS::ECSWorldRegistry::GetInstance().RemoveEntity(EterBase::EntityId(dwVID));
	m_spatialGrid.Remove(EntityVid(dwVID));
	m_aliveActorsMap.erase(dwVID);
	m_subsystemActorRegistry.UnregisterActor(dwVID);

	// 3. Obsluga cyklu zycia w menedzerze sceny i rejestrze swiata
	if (bFadeOut)
	{
		pkInst->DeleteBlendOut();
		m_sceneMgr.MoveToDead(pkInst);
		m_actorRegistry.SetDead(EntityVid(dwVID), true);

		if (IAbstractPlayer::GetSingletonPtr())
		{
			IAbstractPlayer::GetSingleton().NotifyCharacterDead(dwVID);
		}

		if (m_pfnCharacterDeadCallback)
		{
			m_pfnCharacterDeadCallback(dwVID);
		}
	}
	else
	{
		m_sceneMgr.RemoveAliveInstance(pkInst);
		m_sceneMgr.RemoveDeadInstance(pkInst);
		m_actorRegistry.UnregisterActor(EntityVid(dwVID));
		CInstanceBase::Delete(pkInst);
	}

	return true;
}

void CPythonCharacterManager::CentralUpdateActorPosition(DWORD dwVID, float x, float y, float z, float rot, float speed)
{
	m_spatialGrid.UpdateIfMoved(EntityVid(dwVID), x, y);
	m_actorRegistry.UpdatePosition(EntityVid(dwVID), x, y, z, rot);
	UserInterface::ECS::ECSWorldRegistry::GetInstance().GetTransformTable().AddOrUpdate(dwVID, x, y, z, rot, speed);
}

void CPythonCharacterManager::CentralClearActors(bool bClearDead)
{
	m_mainActor = nullptr;
	m_boundActor = nullptr;
	if (m_pickedActor)
	{
		m_pickedActor->OnUnselected();
		m_pickedActor = nullptr;
	}
	m_pickedInstances.clear();
	m_pickedActorScreenPos = D3DXVECTOR2(0.0f, 0.0f);
	m_actorProviderAdapter.SetMainActorVID(0);
	m_actorRegistry.SetMainActorVid(EntityVid(0));

	for (auto& pair : m_aliveActorsMap)
	{
		if (pair.second)
		{
			CInstanceBase::Delete(pair.second);
		}
	}
	m_aliveActorsMap.clear();

	m_subsystemActorRegistry.ClearAll();
	m_sceneMgr.ClearAlive();
	m_spatialGrid.Clear();
	UserInterface::ECS::ECSWorldRegistry::GetInstance().Clear();

	if (bClearDead)
	{
		m_sceneMgr.ClearDead([this](DWORD vid) {
			m_actorRegistry.UnregisterActor(EntityVid(vid));
		});
		m_actorRegistry.Clear();
	}
	else
	{
		for (auto it = m_sceneMgr.GetAliveInstances().begin(); it != m_sceneMgr.GetAliveInstances().end(); ++it)
		{
			if (*it)
				m_actorRegistry.UnregisterActor(EntityVid((*it)->GetVirtualID()));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Managing Process

CInstanceBase* CPythonCharacterManager::CreateInstance(const CInstanceBase::SCreateData& c_rkCreateData)
{
	if (c_rkCreateData.m_dwVID == 0)
		return NULL;

	if (m_subsystemActorRegistry.ContainsActor(c_rkCreateData.m_dwVID)) [[unlikely]]
	{
		TraceError("CPythonCharacterManager::CreateInstance: VID[%d] - ALREADY EXIST\n", c_rkCreateData.m_dwVID);
		return NULL;
	}

	CInstanceBase* pCharacterInstance = CInstanceBase::New();
	if (!CentralRegisterActor(c_rkCreateData.m_dwVID, pCharacterInstance, &c_rkCreateData)) [[unlikely]]
	{
		TraceError("CPythonCharacterManager::CreateInstance: VID[%d] - Central registration failed\n", c_rkCreateData.m_dwVID);
		CInstanceBase::Delete(pCharacterInstance);
		return NULL;
	}

	if (!pCharacterInstance->Create(c_rkCreateData)) [[unlikely]]
	{
		TraceError("CPythonCharacterManager::CreateInstance VID[%d] Race[%d]", c_rkCreateData.m_dwVID, c_rkCreateData.m_dwRace);
		CentralUnregisterActor(c_rkCreateData.m_dwVID, false);
		return NULL;
	}

	return pCharacterInstance;
}

CInstanceBase* CPythonCharacterManager::RegisterInstance(DWORD VirtualID)
{
	if (VirtualID == 0)
		return NULL;

	if (m_subsystemActorRegistry.ContainsActor(VirtualID))
	{
		return NULL;
	}

	CInstanceBase* pCharacterInstance = CInstanceBase::New();
	if (!CentralRegisterActor(VirtualID, pCharacterInstance, nullptr))
	{
		CInstanceBase::Delete(pCharacterInstance);
		return NULL;
	}

	return pCharacterInstance;
}

void CPythonCharacterManager::DeleteInstance(DWORD dwDelVID)
{
	if (!CentralUnregisterActor(dwDelVID, false))
	{
		Tracef("DeleteCharacterInstance: no vid by %d\n", dwDelVID);
	}
}

void CPythonCharacterManager::__DeleteBlendOutInstance(CInstanceBase* pkInstDel)
{
	if (!pkInstDel)
		return;

	CentralUnregisterActor(pkInstDel->GetVirtualID(), true);
}

void CPythonCharacterManager::DeleteInstanceByFade(DWORD dwVID)
{
	CentralUnregisterActor(dwVID, true);
}

void CPythonCharacterManager::DeleteVehicleInstance(DWORD VirtualID)
{
	DeleteInstance(VirtualID);
}

void CPythonCharacterManager::SelectInstance(DWORD VirtualID)
{
	m_boundActor = FindInstancePtr(VirtualID);
	if (!m_boundActor)
	{
		Tracef("SelectCharacterInstance: no vid by %d\n", VirtualID);
	}
}

CInstanceBase* CPythonCharacterManager::FindInstancePtr(DWORD VirtualID)
{
	return m_subsystemActorRegistry.FindActor(VirtualID);
}

CInstanceBase* CPythonCharacterManager::GetInstancePtr(DWORD VirtualID)
{
	return FindInstancePtr(VirtualID);
}

Core::Result<CInstanceBase*, Core::ActorError> CPythonCharacterManager::GetInstanceResult(DWORD VirtualID)
{
	CInstanceBase* pActor = FindInstancePtr(VirtualID);
	if (!pActor)
		return std::unexpected(Core::ActorError::ActorNotFound);
	return pActor;
}

CInstanceBase* CPythonCharacterManager::GetInstancePtrByName(const char* name)
{
	if (!name)
		return NULL;

	for (auto& pair : m_aliveActorsMap)
	{
		CInstanceBase* pInstance = pair.second;
		if (pInstance && !strcmp(pInstance->GetNameString(), name))
			return pInstance;
	}

	return NULL;
}

CInstanceBase* CPythonCharacterManager::GetSelectedInstancePtr()
{
	if (m_boundActor && !m_subsystemActorRegistry.ContainsActor(m_boundActor->GetVirtualID()))
	{
		m_boundActor = NULL;
	}
	return m_boundActor;
}

CInstanceBase* CPythonCharacterManager::FindClickableInstancePtr()
{
	return NULL;
}

void CPythonCharacterManager::__UpdateSortPickedActorList()
{
	__UpdatePickedActorList();
	__SortPickedActorList();
}

void CPythonCharacterManager::__UpdatePickedActorList()
{
	m_pickedInstances.clear();

	for (auto& pair : m_aliveActorsMap)
	{
		CInstanceBase* pkInstEach = pair.second;
		if (pkInstEach->CanPickInstance())
		{
			if (pkInstEach->IsDead())
			{
				if (pkInstEach->IntersectBoundingBox())
					m_pickedInstances.push_back(pkInstEach);
			}
			else
			{
				if (pkInstEach->IntersectDefendingSphere())
					m_pickedInstances.push_back(pkInstEach);
			}
		}
	}
}

struct CInstanceBase_SLessCameraDistance
{
	TPixelPosition m_kPPosEye;

	bool operator() (CInstanceBase* pkInstLeft, CInstanceBase* pkInstRight)
	{
		int nLeftDeadPoint = pkInstLeft->IsDead();
		int nRightDeadPoint = pkInstRight->IsDead();

		if (nLeftDeadPoint < nRightDeadPoint)
			return true;

		if (pkInstLeft->CalculateDistanceSq3d(m_kPPosEye) < pkInstRight->CalculateDistanceSq3d(m_kPPosEye))
			return true;

		return false;
	}
};

void CPythonCharacterManager::__SortPickedActorList()
{
	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera)
		return;

	const D3DXVECTOR3& c_rv3EyePos = pCamera->GetEye();

	CInstanceBase_SLessCameraDistance kLess;
	kLess.m_kPPosEye = TPixelPosition(+c_rv3EyePos.x, -c_rv3EyePos.y, +c_rv3EyePos.z);

	std::sort(m_pickedInstances.begin(), m_pickedInstances.end(), kLess);
}

void CPythonCharacterManager::__NEW_Pick()
{
	__UpdateSortPickedActorList();

	CInstanceBase* pkInstMain = GetMainActorPtr();

	for (auto* pkInstEach : m_pickedInstances)
	{
		if (pkInstEach != pkInstMain && pkInstEach->IntersectBoundingBox())
		{
			if (m_pickedActor && m_pickedActor != pkInstEach)
				m_pickedActor->OnUnselected();

			if (pkInstEach->CanPickInstance())
			{
				m_pickedActor = pkInstEach;
				m_pickedActor->OnSelected();
				return;
			}
		}
	}

	for (auto* pkInstEach : m_pickedInstances)
	{
		if (pkInstEach != pkInstMain)
		{
			if (m_pickedActor && m_pickedActor != pkInstEach)
				m_pickedActor->OnUnselected();

			if (pkInstEach->CanPickInstance())
			{
				m_pickedActor = pkInstEach;
				m_pickedActor->OnSelected();
				return;
			}
		}
	}

	if (pkInstMain && pkInstMain->CanPickInstance())
	{
		if (m_pickedInstances.end() != std::find(m_pickedInstances.begin(), m_pickedInstances.end(), pkInstMain))
		{
			if (m_pickedActor && m_pickedActor != pkInstMain)
				m_pickedActor->OnUnselected();

			m_pickedActor = pkInstMain;
			m_pickedActor->OnSelected();
			return;
		}
	}

	if (m_pickedActor)
	{
		m_pickedActor->OnUnselected();
		m_pickedActor = NULL;
	}
}

void CPythonCharacterManager::__OLD_Pick()
{
	for (auto& pair : m_aliveActorsMap)
	{
		CInstanceBase* pkInstEach = pair.second;
		if (pkInstEach == m_mainActor)
			continue;

		if (pkInstEach->IntersectDefendingSphere())
		{
			if (m_pickedActor && m_pickedActor != pkInstEach)
				m_pickedActor->OnUnselected();

			m_pickedActor = pkInstEach;
			m_pickedActor->OnSelected();
			return;
		}
	}

	if (m_pickedActor)
	{
		m_pickedActor->OnUnselected();
		m_pickedActor = NULL;
	}
}

int CPythonCharacterManager::PickAll()
{
	for (auto& pair : m_aliveActorsMap)
	{
		CInstanceBase* pInstance = pair.second;
		if (pInstance->IntersectDefendingSphere())
			return pInstance->GetVirtualID();
	}

	return -1;
}

CInstanceBase* CPythonCharacterManager::GetCloseInstance(CInstanceBase* pInstance)
{
	if (!pInstance)
		return NULL;

	TPixelPosition kPos;
	pInstance->NEW_GetPixelPosition(&kPos);

	float fMinDistance = 10000.0f;
	CInstanceBase* pCloseInstance = NULL;

	// Przepiecie zapytania na SpatialHashGrid::QueryRadius (O(1) komorki siatki przestrzennej)
	auto candidateIds = m_spatialGrid.QueryRadius(kPos.x, kPos.y, fMinDistance);

	for (const auto& entityId : candidateIds)
	{
		DWORD dwVID = entityId.value();
		if (dwVID == pInstance->GetVirtualID())
			continue;

		CInstanceBase* pTargetInstance = GetInstancePtr(dwVID);
		if (!pTargetInstance)
			continue;

		DWORD dwVirtualNumber = pTargetInstance->GetVirtualNumber();
		if (CPythonNonPlayer::ON_CLICK_EVENT_BATTLE != CPythonNonPlayer::Instance().GetEventType(dwVirtualNumber))
			continue;

		float fDistance = pInstance->GetDistance(pTargetInstance);
		if (fDistance < fMinDistance)
		{
			fMinDistance = fDistance;
			pCloseInstance = pTargetInstance;
		}
	}

	// Zabezpieczenie fallback w razie braku zaindeksowanych wpisow
	if (!pCloseInstance && candidateIds.empty())
	{
		for (auto& pair : m_aliveActorsMap)
		{
			CInstanceBase* pTargetInstance = pair.second;
			if (pTargetInstance == pInstance)
				continue;

			DWORD dwVirtualNumber = pTargetInstance->GetVirtualNumber();
			if (CPythonNonPlayer::ON_CLICK_EVENT_BATTLE != CPythonNonPlayer::Instance().GetEventType(dwVirtualNumber))
				continue;

			float fDistance = pInstance->GetDistance(pTargetInstance);
			if (fDistance < fMinDistance)
			{
				fMinDistance = fDistance;
				pCloseInstance = pTargetInstance;
			}
		}
	}

	return pCloseInstance;
}

void CPythonCharacterManager::RefreshAllPCTextTail()
{
	for (auto itor = CharacterInstanceBegin(); itor != CharacterInstanceEnd(); ++itor)
	{
		CInstanceBase* pInstance = *itor;
		if (!pInstance->IsPC())
			continue;

		pInstance->RefreshTextTail();
	}
}

void CPythonCharacterManager::RefreshAllGuildMark()
{
	for (auto itor = CharacterInstanceBegin(); itor != CharacterInstanceEnd(); ++itor)
	{
		CInstanceBase* pInstance = *itor;
		if (!pInstance->IsPC())
			continue;

		pInstance->ChangeGuild(pInstance->GetGuildID());
		pInstance->RefreshTextTail();
	}
}

void CPythonCharacterManager::DeleteAllInstances()
{
	CentralClearActors(true);
}

void CPythonCharacterManager::DestroyAliveInstanceMap()
{
	CentralClearActors(false);
}

void CPythonCharacterManager::DestroyDeadInstanceList()
{
	m_sceneMgr.ClearDead([this](DWORD vid) {
		m_actorRegistry.UnregisterActor(EntityVid(vid));
	});
}

void CPythonCharacterManager::Destroy()
{
	CentralClearActors(true);
	CInstanceBase::DestroySystem();
	__Initialize();
}

void CPythonCharacterManager::__Initialize()
{
	memset(m_adwPointEffect, 0, sizeof(m_adwPointEffect));
	m_mainActor = NULL;
	m_boundActor = NULL;
	m_pickedActor = NULL;
	m_pickedActorScreenPos = D3DXVECTOR2(0.0f, 0.0f);
	m_pfnCharacterDeadCallback = nullptr;
}

CPythonCharacterManager::CPythonCharacterManager()
	: m_characterPicker(&m_subsystemActorRegistry)
	, m_actorProviderAdapter(&m_subsystemActorRegistry, &m_characterPicker)
{
	__Initialize();
}

CPythonCharacterManager::~CPythonCharacterManager()
{
	Destroy();
}
