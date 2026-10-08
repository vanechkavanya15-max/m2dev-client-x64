#include "stdafx.h"
#include "pythoncharactermanager.h"
#include "PythonBackground.h"
#include "PythonNonPlayer.h"
#include "AbstractPlayer.h"
#include "packet.h"

#include "EterLib/Camera.h"
#include "ECS/ECSWorldRegistry.h"
#include "Core/EventBus.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Frame Process

int CHAR_STAGE_VIEW_BOUND = 200 * 100;

void CPythonCharacterManager::AdjustCollisionWithOtherObjects(CActorInstance* pInst)
{
	if (!pInst->IsPC())
		return;

	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	for (CPythonCharacterManager::CharacterIterator i = rkChrMgr.CharacterInstanceBegin(); i != rkChrMgr.CharacterInstanceEnd(); ++i)
	{
		CInstanceBase* pkInstEach = *i;
		CActorInstance* rkActorEach = pkInstEach->GetGraphicThingInstancePtr();

		if (rkActorEach == pInst)
			continue;

		if (rkActorEach->IsPC() || rkActorEach->IsNPC() || rkActorEach->IsEnemy())
			continue;

		if (pInst->TestPhysicsBlendingCollision(*rkActorEach))
		{
			TPixelPosition curPos;
			pInst->GetPixelPosition(&curPos);
			pInst->SetBlendingPosition(curPos);
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

	CInstanceBase* pkInstSrc = GetInstancePtr(dwVIDSrc);
	if (pkInstSrc)
		pkInstSrc->RefreshTextTail();

	CInstanceBase* pkInstDst = GetInstancePtr(dwVIDDst);
	if (pkInstDst)
		pkInstDst->RefreshTextTail();
}

void CPythonCharacterManager::RemovePVPKey(DWORD dwVIDSrc, DWORD dwVIDDst)
{
	CInstanceBase::RemovePVPKey(dwVIDSrc, dwVIDDst);

	CInstanceBase* pkInstSrc = GetInstancePtr(dwVIDSrc);
	if (pkInstSrc)
		pkInstSrc->RefreshTextTail();

	CInstanceBase* pkInstDst = GetInstancePtr(dwVIDDst);
	if (pkInstDst)
		pkInstDst->RefreshTextTail();
}

void CPythonCharacterManager::ChangeGVG(DWORD dwSrcGuildID, DWORD dwDstGuildID)
{
	for (auto& pair : m_kAliveInstMap)
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
	m_pkInstMain = NULL;
	m_actorRegistry.SetMainActorVid(EntityVid(0));
}

bool CPythonCharacterManager::SetMainInstance(DWORD dwVID)
{
	m_pkInstMain = GetInstancePtr(dwVID);
	if (!m_pkInstMain)
		return false;

	m_actorRegistry.SetMainActorVid(EntityVid(dwVID));
	return true;
}

CInstanceBase* CPythonCharacterManager::GetMainInstancePtr()
{
	return m_pkInstMain;
}

void CPythonCharacterManager::GetInfo(std::string* pstInfo)
{
	pstInfo->append("Actor: ");
	CInstanceBase::GetInfo(pstInfo);

	char szInfo[256];
	sprintf(szInfo, "Container - Live %zd, Dead %zd, Grid %zd",
		m_kAliveInstMap.size(),
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
		if (m_kAliveInstMap.size() < 30)
			isCacheMode = false;
	}
	else
	{
		if (m_kAliveInstMap.size() > 40)
			isCacheMode = true;
	}
	s_isOldCacheMode = isCacheMode;

	return isCacheMode;
}

void CPythonCharacterManager::Update()
{
	CInstanceBase::ResetPerformanceCounter();

	CInstanceBase* pkInstMain = GetMainInstancePtr();
	const float fViewBoundSquared = (CHAR_STAGE_VIEW_BOUND + 10) * (CHAR_STAGE_VIEW_BOUND + 10);

	TCharacterInstanceMap::iterator i = m_kAliveInstMap.begin();
	while (m_kAliveInstMap.end() != i)
	{
		TCharacterInstanceMap::iterator c = i++;

		CInstanceBase* pkInstEach = c->second;
		pkInstEach->Update();

		// Synchronize spatial grid coordinates
		TPixelPosition curPos;
		pkInstEach->NEW_GetPixelPosition(&curPos);
		m_spatialGrid.Update(EntityVid(pkInstEach->GetVirtualID()), curPos.x, curPos.y);

		if (pkInstMain)
		{
			if (pkInstEach->IsForceVisible()) [[unlikely]] {
				continue;
			}

			float fDistanceSquared = pkInstEach->NEW_GetDistanceFromDestInstanceSquared(*pkInstMain);
			if (fDistanceSquared > fViewBoundSquared) [[unlikely]] {
				__DeleteBlendOutInstance(pkInstEach);
				m_kAliveInstMap.erase(c);
			}
		}
	}

	UpdateTransform();
	m_sceneMgr.UpdateDeleting();
	__NEW_Pick();
}

void CPythonCharacterManager::ShowPointEffect(DWORD ePoint, DWORD dwVID)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetMainInstancePtr() : GetInstancePtr(dwVID);
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
	CInstanceBase* pMainInstance = GetMainInstancePtr();
	if (pMainInstance)
	{
		CPythonBackground& rkBG = CPythonBackground::Instance();
		for (auto& pair : m_kAliveInstMap)
		{
			CInstanceBase* pSrcInstance = pair.second;
			pSrcInstance->CheckAdvancing();

			if (pSrcInstance->IsPushing())
				rkBG.CheckAdvancing(pSrcInstance);
		}

		rkBG.CheckAdvancing(m_pkInstMain);
	}

	for (auto& pair : m_kAliveInstMap)
	{
		pair.second->Transform();
	}
}

void CPythonCharacterManager::UpdateDeleting()
{
	m_sceneMgr.UpdateDeleting();
}

void CPythonCharacterManager::Deform()
{
	m_sceneMgr.Deform();
}

bool CPythonCharacterManager::OLD_GetPickedInstanceVID(DWORD* pdwPickedActorID)
{
	if (!m_pkInstPick)
		return false;
		
	*pdwPickedActorID = m_pkInstPick->GetVirtualID();
	return true;
}

CInstanceBase* CPythonCharacterManager::OLD_GetPickedInstancePtr()
{
	return m_pkInstPick;
}

D3DXVECTOR2& CPythonCharacterManager::OLD_GetPickedInstPosReference()
{
	return m_v2PickedInstProjPos;
}

bool CPythonCharacterManager::IsRegisteredVID(DWORD dwVID)
{
	return m_kAliveInstMap.find(dwVID) != m_kAliveInstMap.end();
}

bool CPythonCharacterManager::IsAliveVID(DWORD dwVID)
{
	return m_actorRegistry.IsAlive(EntityVid(dwVID)) || (m_kAliveInstMap.find(dwVID) != m_kAliveInstMap.end());
}

bool CPythonCharacterManager::IsDeadVID(DWORD dwVID)
{
	return m_actorRegistry.IsDead(EntityVid(dwVID)) || m_sceneMgr.IsDead(dwVID);
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

	CInstanceBase* pkPickedInst = OLD_GetPickedInstancePtr();
	if (pkPickedInst)
	{
		const D3DXVECTOR3& c_rv3Position = pkPickedInst->GetGraphicThingInstanceRef().GetPosition();
		CPythonGraphic::Instance().ProjectPosition(c_rv3Position.x, c_rv3Position.y, c_rv3Position.z, &m_v2PickedInstProjPos.x, &m_v2PickedInstProjPos.y);
	}
}

void CPythonCharacterManager::RenderShadowMainInstance()
{
	m_sceneMgr.RenderShadowMainInstance(GetMainInstancePtr());
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
// Managing Process

CInstanceBase* CPythonCharacterManager::CreateInstance(const CInstanceBase::SCreateData& c_rkCreateData)
{
	CInstanceBase* pCharacterInstance = RegisterInstance(c_rkCreateData.m_dwVID);
	if (!pCharacterInstance) [[unlikely]]
	{
		TraceError("CPythonCharacterManager::CreateInstance: VID[%d] - ALREADY EXIST\n", c_rkCreateData.m_dwVID);
		return NULL;
	}

	if (!pCharacterInstance->Create(c_rkCreateData)) [[unlikely]]
	{
		TraceError("CPythonCharacterManager::CreateInstance VID[%d] Race[%d]", c_rkCreateData.m_dwVID, c_rkCreateData.m_dwRace);
		DeleteInstance(c_rkCreateData.m_dwVID);
		return NULL;
	}

	if (c_rkCreateData.m_isMain)
	{
		SelectInstance(c_rkCreateData.m_dwVID);
		m_actorRegistry.SetMainActorVid(EntityVid(c_rkCreateData.m_dwVID));
	}

	// Update spatial tracking and domain registry
	m_actorRegistry.UpdatePosition(
		EntityVid(c_rkCreateData.m_dwVID),
		static_cast<float>(c_rkCreateData.m_lPosX),
		static_cast<float>(c_rkCreateData.m_lPosY),
		0.0f,
		c_rkCreateData.m_fRot
	);
	m_spatialGrid.Update(
		EntityVid(c_rkCreateData.m_dwVID),
		static_cast<float>(c_rkCreateData.m_lPosX),
		static_cast<float>(c_rkCreateData.m_lPosY)
	);

	// Register entity into modern C++23 ECS SoA tables
	(void)UserInterface::ECS::ECSWorldRegistry::GetInstance().RegisterEntity(
		EterBase::EntityId(c_rkCreateData.m_dwVID),
		static_cast<float>(c_rkCreateData.m_lPosX),
		static_cast<float>(c_rkCreateData.m_lPosY),
		0.0f,
		c_rkCreateData.m_fRot,
		0.0f
	);

	return pCharacterInstance;
}

CInstanceBase* CPythonCharacterManager::RegisterInstance(DWORD VirtualID)
{
	TCharacterInstanceMap::iterator itor = m_kAliveInstMap.find(VirtualID);
	if (m_kAliveInstMap.end() != itor)
	{
		return NULL;
	}

	CInstanceBase* pCharacterInstance = CInstanceBase::New();
	m_kAliveInstMap.insert(TCharacterInstanceMap::value_type(VirtualID, pCharacterInstance));

	// Synchronize with modern domain registries and graphic scene manager
	Client::World::ActorRecord record{
		.vid = EntityVid(VirtualID),
		.race = 0,
		.type = 0,
		.x = 0.0f,
		.y = 0.0f,
		.z = 0.0f,
		.rotation = 0.0f,
		.name = "",
		.guildId = 0,
		.empire = 0,
		.isDead = false
	};
	m_actorRegistry.RegisterActor(record);
	m_spatialGrid.Insert(EntityVid(VirtualID), 0.0f, 0.0f);
	m_sceneMgr.AddAliveInstance(pCharacterInstance);

	return pCharacterInstance;
}

void CPythonCharacterManager::DeleteInstance(DWORD dwDelVID)
{
	// Remove from modern C++23 ECS SoA tables and domain subsystems
	(void)UserInterface::ECS::ECSWorldRegistry::GetInstance().RemoveEntity(EterBase::EntityId(dwDelVID));
	m_actorRegistry.UnregisterActor(EntityVid(dwDelVID));
	m_spatialGrid.Remove(EntityVid(dwDelVID));

	TCharacterInstanceMap::iterator itor = m_kAliveInstMap.find(dwDelVID);
	if (m_kAliveInstMap.end() == itor)
	{
		Tracef("DeleteCharacterInstance: no vid by %d\n", dwDelVID);
		return;
	}

	CInstanceBase* pkInstDel = itor->second;

	if (pkInstDel == m_pkInstBind)
		m_pkInstBind = NULL;

	if (pkInstDel == m_pkInstMain)
		m_pkInstMain = NULL;

	if (pkInstDel == m_pkInstPick)
		m_pkInstPick = NULL;

	m_sceneMgr.RemoveAliveInstance(pkInstDel);
	m_sceneMgr.RemoveDeadInstance(pkInstDel);

	CInstanceBase::Delete(pkInstDel);
	m_kAliveInstMap.erase(itor);
}

void CPythonCharacterManager::__DeleteBlendOutInstance(CInstanceBase* pkInstDel)
{
	if (!pkInstDel)
		return;

	const DWORD deadVid = pkInstDel->GetVirtualID();

	pkInstDel->DeleteBlendOut();
	m_sceneMgr.MoveToDead(pkInstDel);

	// Synchronize domain registry and spatial partitioning
	m_actorRegistry.SetDead(EntityVid(deadVid), true);
	m_spatialGrid.Remove(EntityVid(deadVid));

	// Bezpieczne rozgloszenie zdarzenia smierci aktora przez EventBus (decoupling C++23)
	UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::ActorDeadEvent(deadVid));

	// Bezpieczna delegacja (callback dla testow / zewnetrznych listenerow)
	if (m_pfnCharacterDeadCallback)
	{
		m_pfnCharacterDeadCallback(deadVid);
	}

	// Bezpieczne powiadomienie IAbstractPlayer z asercja/sprawdzeniem istnienia singletonu
	if (IAbstractPlayer::GetSingletonPtr())
	{
		IAbstractPlayer::GetSingleton().NotifyCharacterDead(deadVid);
	}
}

void CPythonCharacterManager::DeleteInstanceByFade(DWORD dwVID)
{
	TCharacterInstanceMap::iterator f = m_kAliveInstMap.find(dwVID);
	if (m_kAliveInstMap.end() == f)
	{
		return;
	}
	__DeleteBlendOutInstance(f->second);
	m_kAliveInstMap.erase(f);	
}

void CPythonCharacterManager::SelectInstance(DWORD VirtualID)
{
	TCharacterInstanceMap::iterator itor = m_kAliveInstMap.find(VirtualID);
	if (m_kAliveInstMap.end() == itor)
	{
		Tracef("SelectCharacterInstance: no vid by %d\n", VirtualID);
		return;
	}

	m_pkInstBind = itor->second;
}

CInstanceBase* CPythonCharacterManager::GetInstancePtr(DWORD VirtualID)
{
	TCharacterInstanceMap::iterator itor = m_kAliveInstMap.find(VirtualID);
	if (m_kAliveInstMap.end() == itor)
		return NULL;

	return itor->second;
}

CInstanceBase* CPythonCharacterManager::GetInstancePtrByName(const char* name)
{
	for (auto& pair : m_kAliveInstMap)
	{
		CInstanceBase* pInstance = pair.second;
		if (!strcmp(pInstance->GetNameString(), name))
			return pInstance;
	}

	return NULL;
}

CInstanceBase* CPythonCharacterManager::GetSelectedInstancePtr()
{
	return m_pkInstBind;
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
	m_kVct_pkInstPicked.clear();

	for (auto& pair : m_kAliveInstMap)
	{
		CInstanceBase* pkInstEach = pair.second;
		if (pkInstEach->CanPickInstance())
		{
			if (pkInstEach->IsDead())
			{
				if (pkInstEach->IntersectBoundingBox())
					m_kVct_pkInstPicked.push_back(pkInstEach);
			}
			else
			{
				if (pkInstEach->IntersectDefendingSphere())
					m_kVct_pkInstPicked.push_back(pkInstEach);
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

	std::sort(m_kVct_pkInstPicked.begin(), m_kVct_pkInstPicked.end(), kLess);
}

void CPythonCharacterManager::__NEW_Pick()
{
	__UpdateSortPickedActorList();

	CInstanceBase* pkInstMain = GetMainInstancePtr();

	for (auto* pkInstEach : m_kVct_pkInstPicked)
	{
		if (pkInstEach != pkInstMain && pkInstEach->IntersectBoundingBox())
		{
			if (m_pkInstPick && m_pkInstPick != pkInstEach)
				m_pkInstPick->OnUnselected();

			if (pkInstEach->CanPickInstance())
			{
				m_pkInstPick = pkInstEach;
				m_pkInstPick->OnSelected();
				return;
			}
		}
	}

	for (auto* pkInstEach : m_kVct_pkInstPicked)
	{
		if (pkInstEach != pkInstMain)
		{
			if (m_pkInstPick && m_pkInstPick != pkInstEach)
				m_pkInstPick->OnUnselected();

			if (pkInstEach->CanPickInstance())
			{
				m_pkInstPick = pkInstEach;
				m_pkInstPick->OnSelected();
				return;
			}
		}
	}

	if (pkInstMain && pkInstMain->CanPickInstance())
	{
		if (m_kVct_pkInstPicked.end() != std::find(m_kVct_pkInstPicked.begin(), m_kVct_pkInstPicked.end(), pkInstMain))
		{
			if (m_pkInstPick && m_pkInstPick != pkInstMain)
				m_pkInstPick->OnUnselected();

			m_pkInstPick = pkInstMain;
			m_pkInstPick->OnSelected();
			return;
		}
	}

	if (m_pkInstPick)
	{
		m_pkInstPick->OnUnselected();
		m_pkInstPick = NULL;
	}
}

void CPythonCharacterManager::__OLD_Pick()
{
	for (auto& pair : m_kAliveInstMap)
	{
		CInstanceBase* pkInstEach = pair.second;
		if (pkInstEach == m_pkInstMain)
			continue;

		if (pkInstEach->IntersectDefendingSphere())
		{
			if (m_pkInstPick && m_pkInstPick != pkInstEach)
				m_pkInstPick->OnUnselected();

			m_pkInstPick = pkInstEach;
			m_pkInstPick->OnSelected();
			return;
		}
	}

	if (m_pkInstPick)
	{
		m_pkInstPick->OnUnselected();
		m_pkInstPick = NULL;
	}
}

int CPythonCharacterManager::PickAll()
{
	for (auto& pair : m_kAliveInstMap)
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
		for (auto& pair : m_kAliveInstMap)
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
	DestroyAliveInstanceMap();
	DestroyDeadInstanceList();
}

void CPythonCharacterManager::DestroyAliveInstanceMap()
{
	for (auto& pair : m_kAliveInstMap)
		CInstanceBase::Delete(pair.second);

	m_kAliveInstMap.clear();
	m_sceneMgr.ClearAlive();
}

void CPythonCharacterManager::DestroyDeadInstanceList()
{
	m_sceneMgr.ClearDead();
}

void CPythonCharacterManager::Destroy()
{
	UserInterface::ECS::ECSWorldRegistry::GetInstance().Clear();
	m_actorRegistry.Clear();
	m_spatialGrid.Clear();
	m_sceneMgr.Clear();

	DeleteAllInstances();
	CInstanceBase::DestroySystem();
	__Initialize();
}

void CPythonCharacterManager::__Initialize()
{
	memset(m_adwPointEffect, 0, sizeof(m_adwPointEffect));
	m_pkInstMain = NULL;
	m_pkInstBind = NULL;
	m_pkInstPick = NULL;
	m_v2PickedInstProjPos = D3DXVECTOR2(0.0f, 0.0f);
	m_pfnCharacterDeadCallback = nullptr;
}

CPythonCharacterManager::CPythonCharacterManager()
{
	__Initialize();
}

CPythonCharacterManager::~CPythonCharacterManager()
{
	Destroy();
}
