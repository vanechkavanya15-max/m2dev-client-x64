#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameWorld.h"
#include "PythonNetworkStream.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"
#include "AbstractApplication.h"
#include "ItemManager.h"
#include "InstanceBase.h"
#include "Client/Network/WorldPacketCodec.h"
#include <span>

bool PhaseGameWorldBridge::HandleTime(CPythonNetworkStream* pStream, const TPacketGCTime& pack)
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

bool PhaseGameWorldBridge::HandleDungeon(CPythonNetworkStream* pStream, const TPacketGCDungeon& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::WorldPacketCodec::DecodeDungeon(span);
	if (!res.has_value())
		return false;

	const auto& DungeonPacket = res.value();

	switch (DungeonPacket.subheader)
	{
		case DungeonSub::GC::TIME_ATTACK_START:
		{
			break;
		}
		case DungeonSub::GC::DESTINATION_POSITION:
		{
			unsigned long ulx, uly;
			if (!pStream->Recv(sizeof(ulx), &ulx))
				return false;
			if (!pStream->Recv(sizeof(uly), &uly))
				return false;

			CPythonPlayer::Instance().SetDungeonDestinationPosition(ulx, uly);
			break;
		}
		default:
			TraceError("PhaseGameWorldBridge::HandleDungeon: unknown subheader %d", DungeonPacket.subheader);
			break;
	}

	return true;
}

bool PhaseGameWorldBridge::HandleFishing(CPythonNetworkStream* pStream, const TPacketGCFishing& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::WorldPacketCodec::DecodeFishing(span);
	if (!res.has_value())
		return false;

	const auto& FishingPacket = res.value();

	CInstanceBase* pFishingInstance = nullptr;
	if (FishingSub::GC::FISH != FishingPacket.subheader)
	{
		pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(FishingPacket.info);
		if (!pFishingInstance)
			return true;
	}

	switch (FishingPacket.subheader)
	{
		case FishingSub::GC::START:
			pFishingInstance->StartFishing(static_cast<float>(FishingPacket.dir) * 5.0f);
			break;
		case FishingSub::GC::STOP:
			if (pFishingInstance->IsFishing())
				pFishingInstance->StopFishing();
			break;
		case FishingSub::GC::REACT:
			if (pFishingInstance->IsFishing())
			{
				pFishingInstance->SetFishEmoticon();
				pFishingInstance->ReactFishing();
			}
			break;
		case FishingSub::GC::SUCCESS:
			pFishingInstance->CatchSuccess();
			break;
		case FishingSub::GC::FAIL:
			pFishingInstance->CatchFail();
			if (pFishingInstance == CPythonCharacterManager::Instance().GetMainActorPtr())
			{
				PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnFishingFailure", Py_BuildValue("()"));
			}
			break;
		case FishingSub::GC::FISH:
		{
			DWORD dwFishID = FishingPacket.info;
			if (0 == FishingPacket.info)
			{
				PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnFishingNotifyUnknown", Py_BuildValue("()"));
				return true;
			}

			CItemData* pItemData;
			if (!CItemManager::Instance().GetItemDataPointer(dwFishID, &pItemData))
				return true;

			CInstanceBase* pMainInstance = CPythonCharacterManager::Instance().GetMainActorPtr();
			if (!pMainInstance)
				return true;

			if (pMainInstance->IsFishing())
			{
				PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnFishingNotify", Py_BuildValue("(is)", CItemData::ITEM_TYPE_FISH == pItemData->GetType(), pItemData->GetName()));
			}
			else
			{
				PyCallClassMemberFunc(pStream->GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME), "OnFishingSuccess", Py_BuildValue("(is)", CItemData::ITEM_TYPE_FISH == pItemData->GetType(), pItemData->GetName()));
			}
			break;
		}
		default:
			TraceError("PhaseGameWorldBridge::HandleFishing: unknown subheader %d", FishingPacket.subheader);
			break;
	}

	return true;
}

bool PhaseGameWorldBridge::HandleChannel(CPythonNetworkStream* pStream, const TPacketGCChannel& pack)
{
	if (!pStream)
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
	auto res = Client::Network::WorldPacketCodec::DecodeChannel(span);
	if (!res.has_value())
		return false;

	return true;
}
