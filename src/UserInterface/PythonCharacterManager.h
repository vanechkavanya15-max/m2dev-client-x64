#pragma once

#include <functional>
#include <map>
#include <list>
#include <vector>
#include <string>

#include "AbstractCharacterManager.h"
#include "InstanceBase.h"
#include "GameLib/PhysicsObject.h"

#include "World/ActorRegistry.h"
#include "World/SpatialHashGrid.h"
#include "InstanceSceneManager.h"

class CPythonCharacterManager : public CSingleton<CPythonCharacterManager>, public IAbstractCharacterManager, public IObjectManager
{
	public:
		// Character List
		typedef std::list<CInstanceBase *>			TCharacterInstanceList;
		typedef std::map<DWORD, CInstanceBase *>	TCharacterInstanceMap;

		using TCharacterDeadCallback = std::function<void(DWORD)>;
		void SetCharacterDeadCallback(TCharacterDeadCallback callback) { m_pfnCharacterDeadCallback = callback; }

		class CharacterIterator;

	public:
		CPythonCharacterManager();
		virtual ~CPythonCharacterManager();

		virtual void AdjustCollisionWithOtherObjects(CActorInstance* pInst ); 

		void EnableSortRendering(bool isEnable);

		bool IsRegisteredVID(DWORD dwVID);
		bool IsAliveVID(DWORD dwVID);
		bool IsDeadVID(DWORD dwVID);
		bool IsCacheMode();

		bool OLD_GetPickedInstanceVID(DWORD* pdwPickedActorID);
		CInstanceBase* OLD_GetPickedInstancePtr();
		D3DXVECTOR2& OLD_GetPickedInstPosReference();

		CInstanceBase* FindClickableInstancePtr();

		void InsertPVPKey(DWORD dwVIDSrc, DWORD dwVIDDst);
		void RemovePVPKey(DWORD dwVIDSrc, DWORD dwVIDDst);
		void ChangeGVG(DWORD dwSrcGuildID, DWORD dwDstGuildID);

		void GetInfo(std::string* pstInfo);

		void ClearMainInstance();
		bool SetMainInstance(DWORD dwVID);
		CInstanceBase* GetMainInstancePtr();

		void								SCRIPT_SetAffect(DWORD dwVID, DWORD eAffect, BOOL isVisible);
		void								SetEmoticon(DWORD dwVID, DWORD eEmoticon);
		bool								IsPossibleEmoticon(DWORD dwVID);
		void								ShowPointEffect(DWORD dwVID, DWORD ePoint);
		bool								RegisterPointEffect(DWORD ePoint, const char* c_szFileName);

		// System
		void								Destroy();

		void								DeleteAllInstances();

		bool								CreateDeviceObjects();
		void								DestroyDeviceObjects();

		void								Update();
		void								Deform();
		void								Render();
		void								RenderShadowMainInstance();
		void								RenderShadowAllInstances();
		void								RenderCollision();

		// Create/Delete Instance
		CInstanceBase *						CreateInstance(const CInstanceBase::SCreateData& c_rkCreateData);
		CInstanceBase *						RegisterInstance(DWORD VirtualID);

		void								DeleteInstance(DWORD VirtualID);
		void								DeleteInstanceByFade(DWORD VirtualID);
		void								DeleteVehicleInstance(DWORD VirtualID);

		void 								DestroyAliveInstanceMap();
		void 								DestroyDeadInstanceList();

		inline CharacterIterator			CharacterInstanceBegin() { return CharacterIterator(m_kAliveInstMap.begin());}
		inline CharacterIterator			CharacterInstanceEnd() { return CharacterIterator(m_kAliveInstMap.end());}

		// Access Instance
		void								SelectInstance(DWORD VirtualID);
		CInstanceBase *						GetSelectedInstancePtr();

		CInstanceBase *						GetInstancePtr(DWORD VirtualID);
		CInstanceBase *						GetInstancePtrByName(const char *name);

		// Pick		
		int									PickAll();
		CInstanceBase *						GetCloseInstance(CInstanceBase * pInstance);

		// Subsystem coordinators
		[[nodiscard]] Client::World::ActorRegistry& GetActorRegistry() noexcept { return m_actorRegistry; }
		[[nodiscard]] const Client::World::ActorRegistry& GetActorRegistry() const noexcept { return m_actorRegistry; }
		[[nodiscard]] Client::World::SpatialHashGrid& GetSpatialHashGrid() noexcept { return m_spatialGrid; }
		[[nodiscard]] const Client::World::SpatialHashGrid& GetSpatialHashGrid() const noexcept { return m_spatialGrid; }
		[[nodiscard]] UserInterface::InstanceSceneManager& GetSceneManager() noexcept { return m_sceneMgr; }
		[[nodiscard]] const UserInterface::InstanceSceneManager& GetSceneManager() const noexcept { return m_sceneMgr; }

		// Refresh TextTail
		void								RefreshAllPCTextTail();
		void								RefreshAllGuildMark();

	protected:
		void								UpdateTransform();
		void								UpdateDeleting();

	protected:
		void __Initialize();

		void __DeleteBlendOutInstance(CInstanceBase* pkInstDel);

		void __OLD_Pick();
		void __NEW_Pick();

		void __UpdateSortPickedActorList();
		void __UpdatePickedActorList();
		void __SortPickedActorList();

		void __RenderSortedAliveActorList();
		void __RenderSortedDeadActorList();

	protected:
		CInstanceBase *						m_pkInstMain;
		CInstanceBase *						m_pkInstPick;
		CInstanceBase *						m_pkInstBind;
		D3DXVECTOR2							m_v2PickedInstProjPos;

		TCharacterInstanceMap				m_kAliveInstMap;

		std::vector<CInstanceBase*>			m_pickedInstances;

		DWORD								m_adwPointEffect[POINT_MAX_NUM];

		TCharacterDeadCallback				m_pfnCharacterDeadCallback;

		// Subsystems
		Client::World::ActorRegistry		m_actorRegistry;
		Client::World::SpatialHashGrid		m_spatialGrid;
		UserInterface::InstanceSceneManager	m_sceneMgr;

	public:
		class CharacterIterator
		{
		public:
			CharacterIterator(){}
			CharacterIterator(const TCharacterInstanceMap::iterator & it) : m_it(it) {}

			inline CInstanceBase * operator * () {	return m_it->second; }

			inline CharacterIterator & operator ++()
			{
				++m_it;
				return *this;
			}

			inline CharacterIterator operator ++(int)
			{
				CharacterIterator new_it = *this;
				++(*this);
				return new_it;
			}

			inline CharacterIterator & operator = (const CharacterIterator & rhs)
			{
				m_it = rhs.m_it;
				return (*this);
			}

			inline bool operator == (const CharacterIterator & rhs) const
			{
				return m_it == rhs.m_it;
			}

			inline bool operator != (const CharacterIterator & rhs) const
			{
				return m_it != rhs.m_it;
			}

			private:
				TCharacterInstanceMap::iterator m_it;
		};
};
