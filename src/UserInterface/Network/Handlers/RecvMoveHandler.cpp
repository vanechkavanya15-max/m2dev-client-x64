#include "../../StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../NetworkActorManager.h"

/**
 * @file RecvMoveHandler.cpp
 * @brief Handler for receiving and processing movement packets from the server.
 */

/**
 * @brief Handles the reception of a character movement packet.
 * 
 * This function reads the movement packet from the network stream, validates it,
 * converts the global coordinates to local coordinates, and updates the actor's
 * movement data in the Network Actor Manager.
 * 
 * @return true if the packet was successfully received and processed.
 * @return false if there was an error reading the packet.
 */
bool CPythonNetworkStream::RecvCharacterMovePacket()
{
	TPacketGCMove movePacket;
	if (!Recv(sizeof(TPacketGCMove), &movePacket))
	{
		Tracen("CPythonNetworkStream::RecvCharacterMovePacket - PACKET READ ERROR");
		return false;
	}

	__GlobalPositionToLocalPosition(movePacket.lX, movePacket.lY);

	SNetworkMoveActorData netMoveActorData;
	netMoveActorData.m_dwArg = movePacket.bArg;
	netMoveActorData.m_dwFunc = movePacket.bFunc;
	netMoveActorData.m_dwTime = movePacket.dwTime;
	netMoveActorData.m_dwVID = movePacket.dwVID;
	netMoveActorData.m_fRot = movePacket.bRot * 5.0f;
	netMoveActorData.m_lPosX = movePacket.lX;
	netMoveActorData.m_lPosY = movePacket.lY;
	netMoveActorData.m_dwDuration = movePacket.dwDuration;

	m_rokNetActorMgr->MoveActor(netMoveActorData);

	return true;
}
