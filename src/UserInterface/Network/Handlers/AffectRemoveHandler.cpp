#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include "../../PythonPlayer.h"

/**
 * @brief Processes the AFFECT_REMOVE packet from the game server.
 * 
 * This method reads the TPacketGCAffectRemove structure to identify which affect
 * has expired or been dispelled, and subsequently updates the local client's
 * C++ memory state by resetting the corresponding affect in CPythonPlayer.
 * 
 * @return true if the packet was successfully received and processed, false otherwise.
 */
bool CPythonNetworkStream::RecvAffectRemovePacket()
{
	TPacketGCAffectRemove packet;
	
	if (!Recv(sizeof(packet), &packet))
	{
		return false;
	}

	// Update the C++ memory state to remove the affect.
	// This handles deactivating skill slots for toggle skills if applicable.
	CPythonPlayer::Instance().ResetAffect(packet.dwType);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_NEW_RemoveAffect", Py_BuildValue("(ii)", packet.dwType, packet.bApplyOn));

	return true;
}
