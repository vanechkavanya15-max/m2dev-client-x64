#include "StdAfx.h"
#include <cstdint>
#include "../../PythonNetworkStream.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include "../../Packet.h"

/**
 * @file PvpHandler.cpp
 * @brief Handles incoming PVP (Player vs Player) network packets from the server.
 *
 * Adheres to modern C++20 standard, removes Hungarian notation, and decouples
 * GUI logic directly from network packet processing by operating exclusively on
 * local memory state (e.g. CPythonCharacterManager, CPythonPlayer).
 */

/**
 * @brief Processes the PVP packet to update character PVP statuses and relationships.
 * 
 * Extracts source and destination Virtual IDs (VIDs) and updates PVP keys and
 * duel states based on the specific mode provided by the server.
 * 
 * @return true if the packet was successfully received and processed.
 * @return false if there was an error reading the packet.
 */
bool CPythonNetworkStream::RecvPVPPacket()
{
	TPacketGCPVP pvpPacket;
	if (!Recv(sizeof(pvpPacket), &pvpPacket))
		return false;

	CPythonCharacterManager& chrMgr = CPythonCharacterManager::Instance();
	CPythonPlayer& player = CPythonPlayer::Instance();

	const uint32_t sourceId = pvpPacket.dwVIDSrc;
	const uint32_t targetId = pvpPacket.dwVIDDst;

	switch (pvpPacket.bMode)
	{
		case PVP_MODE_AGREE:
			chrMgr.RemovePVPKey(sourceId, targetId);

			if (player.IsMainCharacterIndex(targetId))
				player.RememberChallengeInstance(sourceId);

			if (player.IsMainCharacterIndex(sourceId))
				player.RememberCantFightInstance(targetId);
			break;

		case PVP_MODE_REVENGE:
		{
			chrMgr.RemovePVPKey(sourceId, targetId);

			const uint32_t killerId = sourceId;
			const uint32_t victimId = targetId;

			if (player.IsMainCharacterIndex(victimId))
				player.RememberRevengeInstance(killerId);

			if (player.IsMainCharacterIndex(killerId))
				player.RememberCantFightInstance(victimId);
			break;
		}

		case PVP_MODE_FIGHT:
			chrMgr.InsertPVPKey(sourceId, targetId);
			player.ForgetInstance(sourceId);
			player.ForgetInstance(targetId);
			break;

		case PVP_MODE_NONE:
			chrMgr.RemovePVPKey(sourceId, targetId);
			player.ForgetInstance(sourceId);
			player.ForgetInstance(targetId);
			break;
	}

	__RefreshTargetBoardByVID(sourceId);
	__RefreshTargetBoardByVID(targetId);

	return true;
}
