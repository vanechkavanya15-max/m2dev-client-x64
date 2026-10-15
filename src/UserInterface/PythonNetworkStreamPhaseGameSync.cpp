#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameSync.h"
#include "PythonNetworkStream.h"
#include "PythonBackground.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonItem.h"
#include "AbstractApplication.h"
#include "AbstractCharacterManager.h"
#include "AbstractPlayer.h"
#include "InstanceBase.h"
#include "NetworkActorManager.h"
#include "GameLib/FlyingObjectManager.h"
#include "EffectLib/EffectManager.h"
#include "PythonTextTail.h"
#include "Client/Network/WorldPacketCodec.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include <span>
#include <string>
#include <string_view>

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

	if (pack.wPort == 0)
	{
		Warp(pStream, pack.lX, pack.lY);
		return true;
	}

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

	CPythonCharacterManager::Instance().DeleteAllInstances();
	CPythonItem::Instance().DeleteAllItems();
	CPythonTextTail::Instance().Clear();
	CFlyingManager::Instance().DeleteAllInstances();
	CEffectManager::Instance().DeleteAllInstances();

	if (!pStream->m_rokNetActorMgr.IsNull())
	{
		pStream->m_rokNetActorMgr->Destroy();
		pStream->m_rokNetActorMgr->SetMainActorVID(pStream->GetMainActorVID());
	}

	if (CPythonPlayer::InstancePtr())
	{
		CPythonPlayer::Instance().SetAttackKeyState(false);
		CPythonPlayer::Instance().NEW_Stop();
	}
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

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	std::string_view driverName = pDriver ? pDriver->GetDriverName() : "standard_x64";

	if (driverName == "luria")
	{
		uint8_t pingHeader = 0;
		if (!pStream->Recv(sizeof(pingHeader), &pingHeader))
			return false;

		pStream->m_dwLastGamePingTime = ELTimer_GetMSec();

		uint8_t pongHeader = Luria::CG::PONG; // 0xFE (1B)
		if (!pStream->Send(sizeof(pongHeader), &pongHeader))
			return false;

		pStream->__SendInternalBuffer();
		return true;
	}
	else if (driverName == "beavium")
	{
		uint8_t pingHeader = 0;
		if (!pStream->Recv(sizeof(pingHeader), &pingHeader))
			return false;

		pStream->m_dwLastGamePingTime = ELTimer_GetMSec();

		uint8_t pongHeader = Beavium::CG::PONG; // 0xFE (1B)
		if (!pStream->Send(sizeof(pongHeader), &pongHeader))
			return false;

		pStream->__SendInternalBuffer();
		return true;
	}

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

	if (!pStream->Send(sizeof(kPacketPong), &kPacketPong))
		return false;

	pStream->__SendInternalBuffer();
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

	if ("Game" == pStream->m_strPhase)
		return;

	pStream->m_phaseLeaveFunc.Run();

	pStream->m_strPhase = "Game";
	pStream->RefreshProtocolHandlers();

	pStream->m_dwChangingPhaseTime = ELTimer_GetMSec();
	pStream->m_phaseProcessFunc.Set(pStream, &CPythonNetworkStream::GamePhase);
	pStream->m_phaseLeaveFunc.Set(pStream, &CPythonNetworkStream::__LeaveGamePhase);

	IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
	rkPlayer.SetMainCharacterIndex(pStream->GetMainActorVID());

	pStream->__RefreshStatus();

	if (pStream->m_poHandler && !pStream->m_apoPhaseWnd[CPythonNetworkStream::PHASE_WINDOW_GAME])
	{
		PyCallClassMemberFunc(pStream->m_poHandler, "SetGamePhase", Py_BuildValue("()"));
	}
}

