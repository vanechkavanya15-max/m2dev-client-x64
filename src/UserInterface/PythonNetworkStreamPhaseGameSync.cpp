#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameSync.h"
#include "PythonNetworkStream.h"
#include "PythonBackground.h"
#include "PythonPlayer.h"
#include "PythonItem.h"
#include "AbstractApplication.h"
#include "AbstractCharacterManager.h"
#include "AbstractPlayer.h"
#include "InstanceBase.h"
#include "Client/Network/WorldPacketCodec.h"
#include <span>
#include <string>

bool PhaseGameSyncBridge::HandleWarp(CPythonNetworkStream* pStream)
{
	if (!pStream)
		return false;

	TPacketGCWarp kWarpPacket;
	if (!pStream->Recv(sizeof(kWarpPacket), &kWarpPacket))
		return false;

	return HandleWarp(pStream, kWarpPacket);
}

bool PhaseGameSyncBridge::HandleWarp(CPythonNetworkStream* pStream, const TPacketGCWarp& pack)
{
	if (!pStream)
		return false;

	pStream->__DirectEnterMode_Set(pStream->m_dwSelectedCharacterIndex);
	pStream->Connect((DWORD)pack.lAddr, pack.wPort);

	return true;
}

void PhaseGameSyncBridge::Warp(CPythonNetworkStream* pStream, LONG lGlobalX, LONG lGlobalY)
{
	if (!pStream)
		return;

	CPythonBackground& rkBgMgr = CPythonBackground::Instance();
	rkBgMgr.Destroy();
	rkBgMgr.Create();
	rkBgMgr.Warp(lGlobalX, lGlobalY);
	rkBgMgr.RefreshShadowLevel();

	// NOTE : Warp 했을때 CenterPosition의 Height가 0이기 때문에 카메라가 땅바닥에 박혀있게 됨
	//        움직일때마다 Height가 갱신 되기 때문이므로 맵을 이동하면 Position을 강제로 한번
	//        셋팅해준다 - [levites]
	int32_t lLocalX = lGlobalX;
	int32_t lLocalY = lGlobalY;
	pStream->__GlobalPositionToLocalPosition(lLocalX, lLocalY);
	float fHeight = CPythonBackground::Instance().GetHeight(float(lLocalX), float(lLocalY));

	IAbstractApplication& rkApp = IAbstractApplication::GetSingleton();
	rkApp.SetCenterPosition(float(lLocalX), float(lLocalY), fHeight);

	ShowMapName(pStream, lLocalX, lLocalY);
}

void PhaseGameSyncBridge::ShowMapName(CPythonNetworkStream* pStream, LONG lLocalX, LONG lLocalY)
{
	if (!pStream)
		return;

	const std::string& c_rstrMapFileName = CPythonBackground::Instance().GetWarpMapName();
	PyCallClassMemberFunc(pStream->m_apoPhaseWnd[CPythonNetworkStream::PHASE_WINDOW_GAME], "ShowMapName", Py_BuildValue("(sii)", c_rstrMapFileName.c_str(), lLocalX, lLocalY));
}

bool PhaseGameSyncBridge::HandleTime(CPythonNetworkStream* pStream)
{
	if (!pStream)
		return false;

	TPacketGCTime TimePacket;
	if (!pStream->Recv(sizeof(TimePacket), &TimePacket))
		return false;

	return HandleTime(pStream, TimePacket);
}

bool PhaseGameSyncBridge::HandleTime(CPythonNetworkStream* pStream, const TPacketGCTime& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::WorldPacketCodec::DecodeTime(span);
	if (!res.has_value())
		return false;

	IAbstractApplication& rkApp = IAbstractApplication::GetSingleton();
	rkApp.SetServerTime(res.value().time);

	return true;
}

bool PhaseGameSyncBridge::HandleTimeStatus(CPythonNetworkStream* pStream)
{
	return HandleTime(pStream);
}

bool PhaseGameSyncBridge::HandleTimeStatus(CPythonNetworkStream* pStream, const TPacketGCTime& pack)
{
	return HandleTime(pStream, pack);
}

bool PhaseGameSyncBridge::HandlePing(CPythonNetworkStream* pStream)
{
	if (!pStream)
		return false;

	TPacketGCPing kPacketPing;
	if (!pStream->Recv(sizeof(TPacketGCPing), &kPacketPing))
		return false;

	return HandlePing(pStream, kPacketPing);
}

bool PhaseGameSyncBridge::HandlePing(CPythonNetworkStream* pStream, const TPacketGCPing& pack)
{
	if (!pStream)
		return false;

	pStream->m_dwLastGamePingTime = ELTimer_GetMSec();

	// Sync server time from ping
	ELTimer_SetServerMSec(pack.server_time);

	TPacketCGPong kPacketPong;
	kPacketPong.header = CG::PONG;
	kPacketPong.length = sizeof(kPacketPong);

	if (!pStream->Send(sizeof(TPacketCGPong), &kPacketPong))
		return false;

	return true;
}

void PhaseGameSyncBridge::LeaveGamePhase(CPythonNetworkStream* pStream)
{
	if (!pStream)
		return;

	CInstanceBase::ClearPVPKeySystem();

	pStream->__ClearNetworkActorManager();

	pStream->m_bComboSkillFlag = FALSE;

	IAbstractCharacterManager& rkChrMgr = IAbstractCharacterManager::GetSingleton();
	rkChrMgr.Destroy();

	CPythonItem& rkItemMgr = CPythonItem::Instance();
	rkItemMgr.Destroy();
}

void PhaseGameSyncBridge::SetGamePhase(CPythonNetworkStream* pStream)
{
	if (!pStream)
		return;

	if ("Game" != pStream->m_strPhase)
		pStream->m_phaseLeaveFunc.Run();

	pStream->m_strPhase = "Game";

	pStream->m_dwChangingPhaseTime = ELTimer_GetMSec();
	pStream->m_phaseProcessFunc.Set(pStream, &CPythonNetworkStream::GamePhase);
	pStream->m_phaseLeaveFunc.Set(pStream, &CPythonNetworkStream::__LeaveGamePhase);

	IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
	rkPlayer.SetMainCharacterIndex(pStream->GetMainActorVID());

	pStream->__RefreshStatus();
}
