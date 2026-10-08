#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonNetworkStreamPhaseGameTarget.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonNetworkStream.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonMiniMap.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonBackground.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "PythonCharacterManager.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "InstanceBase.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "../GameLib/FlyingObjectManager.h"
#endif
#ifndef TEST_MODE_DISABLE_STDAFX
#include "../Client/Network/TargetPacketCodec.h"
#endif

bool PhaseGameTargetBridge::HandleTargetCreate(CPythonNetworkStream* pStream, const TPacketGCTargetCreate& pack)
{
	if (!pStream)
		return false;

	// In the real code, we get a buffer and decode it. Since the function signature 
	// provided by the prompt takes the legacy packet, we will construct a buffer
	// from it and then pass it to the codec.
	auto result = Client::Network::TargetPacketCodec::DecodeTargetCreate(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack)));
	if (!result.has_value())
		return false;

	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	rkpyMiniMap.CreateTarget(result->lID, result->szTargetName);

	PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "BINARY_OpenAtlasWindow", Py_BuildValue("()"));
	
	return true;
}

bool PhaseGameTargetBridge::HandleTargetUpdate(CPythonNetworkStream* pStream, const TPacketGCTargetUpdate& pack)
{
	if (!pStream)
		return false;

	auto result = Client::Network::TargetPacketCodec::DecodeTargetUpdate(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack)));
	if (!result.has_value())
		return false;

	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	rkpyMiniMap.UpdateTarget(result->lID, result->lX, result->lY);

	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	rkpyBG.CreateTargetEffect(result->lID, result->lX, result->lY);

	return true;
}

bool PhaseGameTargetBridge::HandleTargetDelete(CPythonNetworkStream* pStream, const TPacketGCTargetDelete& pack)
{
	if (!pStream)
		return false;

	auto result = Client::Network::TargetPacketCodec::DecodeTargetDelete(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack)));
	if (!result.has_value())
		return false;

	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	rkpyMiniMap.DeleteTarget(result->lID);

	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	rkpyBG.DeleteTargetEffect(result->lID);

	return true;
}

bool PhaseGameTargetBridge::HandleCreateFly(CPythonNetworkStream* pStream, const TPacketGCCreateFly& pack)
{
	if (!pStream)
		return false;

	auto result = Client::Network::TargetPacketCodec::DecodeCreateFly(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack)));
	if (!result.has_value())
		return false;

	CFlyingManager& rkFlyMgr = CFlyingManager::Instance();
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();

	CInstanceBase * pkStartInst = rkChrMgr.GetInstancePtr(result->dwStartVID);
	CInstanceBase * pkEndInst = rkChrMgr.GetInstancePtr(result->dwEndVID);
	if (!pkStartInst || !pkEndInst)
		return true;

	rkFlyMgr.CreateIndexedFly(result->bType, pkStartInst->GetGraphicThingInstancePtr(), pkEndInst->GetGraphicThingInstancePtr());

	return true;
}
