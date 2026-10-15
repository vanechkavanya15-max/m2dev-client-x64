#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameMisc.h"
#include "PythonNetworkStream.h"
#include "PythonNetworkStreamPhaseGameSync.h"
#include "PythonNetworkStreamPhaseGameWorld.h"
#include "PythonMiniMap.h"
#include "PythonCharacterManager.h"
#include "AbstractCharacterManager.h"
#include "InstanceBase.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "../GameLib/RaceMotionData.h"

//////////////////////////////////////////////////////////////////////////
// Observer Packets

bool CPythonNetworkStream::RecvObserverAddPacket()
{
	TPacketGCObserverAdd kObserverAddPacket;
	if (!Recv(sizeof(kObserverAddPacket), &kObserverAddPacket))
		return false;

	CPythonMiniMap::Instance().AddObserver(
		kObserverAddPacket.vid, 
		kObserverAddPacket.x * 100.0f, 
		kObserverAddPacket.y * 100.0f
	);

	return true;
}

bool CPythonNetworkStream::RecvObserverRemovePacket()
{
	TPacketGCObserverRemove kObserverRemovePacket;
	if (!Recv(sizeof(kObserverRemovePacket), &kObserverRemovePacket))
		return false;

	CPythonMiniMap::Instance().RemoveObserver(kObserverRemovePacket.vid);

	return true;
}

bool CPythonNetworkStream::RecvObserverMovePacket()
{
	TPacketGCObserverMove kObserverMovePacket;
	if (!Recv(sizeof(kObserverMovePacket), &kObserverMovePacket))
		return false;

	CPythonMiniMap::Instance().MoveObserver(
		kObserverMovePacket.vid, 
		kObserverMovePacket.x * 100.0f, 
		kObserverMovePacket.y * 100.0f
	);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Warp Packets

bool CPythonNetworkStream::RecvWarpPacket()
{
	return PhaseGameSyncBridge::HandleWarp(this);
}

bool CPythonNetworkStream::RecvWarpPacketLuria()
{
	Luria::TPacketGCWarpLuria warp{};
	if (!Recv(sizeof(warp), &warp))
		return false;

	if (warp.wPort == 0)
	{
		PhaseGameSyncBridge::Warp(this, warp.lX, warp.lY);
	}
	else
	{
		__DirectEnterMode_Set(m_dwSelectedCharacterIndex);
		Connect(static_cast<DWORD>(warp.lAddr), warp.wPort);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Fishing Packets

bool CPythonNetworkStream::SendFishingPacket(int iRotation)
{
	TPacketCGFishing pack;
	pack.header = CG::FISHING;
	pack.length = sizeof(pack);
	pack.dir = iRotation / 5;

	if (!Send(sizeof(pack), &pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGiveItemPacket(DWORD dwTargetVID, TItemPos ItemPos, int iItemCount)
{
	TPacketCGGiveItem GiveItemPacket;
	GiveItemPacket.header = CG::ITEM_GIVE;
	GiveItemPacket.length = sizeof(GiveItemPacket);
	GiveItemPacket.dwTargetVID = dwTargetVID;
	GiveItemPacket.ItemPos = ItemPos;
	GiveItemPacket.byItemCount = iItemCount;

	if (!Send(sizeof(GiveItemPacket), &GiveItemPacket))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvFishing()
{
	TPacketGCFishing FishingPacket;
	if (!Recv(sizeof(FishingPacket), &FishingPacket))
		return false;

	return PhaseGameWorldBridge::HandleFishing(this, FishingPacket);
}

//////////////////////////////////////////////////////////////////////////
// Dungeon Packets

bool CPythonNetworkStream::RecvDungeon()
{
	TPacketGCDungeon DungeonPacket;
	if (!Recv(sizeof(DungeonPacket), &DungeonPacket))
		return false;

	return PhaseGameWorldBridge::HandleDungeon(this, DungeonPacket);
}

//////////////////////////////////////////////////////////////////////////
// Time Sync Packets

bool CPythonNetworkStream::RecvTimePacket()
{
	return PhaseGameSyncBridge::HandleTime(this);
}

bool CPythonNetworkStream::RecvTimeStatusPacket()
{
	return PhaseGameSyncBridge::HandleTimeStatus(this);
}

//////////////////////////////////////////////////////////////////////////
// Dig Motion Packets

bool CPythonNetworkStream::RecvDigMotionPacket()
{
	TPacketGCDigMotion kDigMotion;
	if (!Recv(sizeof(kDigMotion), &kDigMotion))
		return false;

#ifdef _DEBUG
	Tracef(" Dig Motion [%d/%d]\n", kDigMotion.vid, kDigMotion.count);
#endif

	IAbstractCharacterManager& rkChrMgr = IAbstractCharacterManager::GetSingleton();
	CInstanceBase* pkInstMain = rkChrMgr.GetInstancePtr(kDigMotion.vid);
	CInstanceBase* pkInstTarget = rkChrMgr.GetInstancePtr(kDigMotion.target_vid);
	if (NULL == pkInstMain)
		return true;

	if (pkInstTarget)
		pkInstMain->NEW_LookAtDestInstance(*pkInstTarget);

	for (int i = 0; i < kDigMotion.count; ++i)
		pkInstMain->PushOnceMotion(CRaceMotionData::NAME_DIG);

	return true;
}
