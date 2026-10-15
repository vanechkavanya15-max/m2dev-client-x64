#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonCharacterManager.h"
#include "../../InstanceBase.h"
#include "../../Packet.h"

/**
 * @brief Handles the incoming Change Speed packet from the server.
 * 
 * This packet is used to update the moving and attack speed of a specific character
 * (identified by their Virtual ID) in the game world. It retrieves the 
 * character instance and applies the newly received speeds.
 * 
 * @return bool Returns true if the packet was processed successfully, false on error.
 */
bool CPythonNetworkStream::RecvChangeSpeedPacket()
{
	TPacketGCChangeSpeed speedPacket;

	if (!Recv(sizeof(TPacketGCChangeSpeed), &speedPacket))
	{
		Tracen("RecvChangeSpeedPacket: Failed to receive packet data.");
		return false;
	}

	CInstanceBase* instance = CPythonCharacterManager::Instance().GetInstancePtr(speedPacket.vid);

	if (!instance)
	{
		// Instance not found, but packet was correctly received, so return true.
		return true;
	}

	instance->SetMoveSpeed(speedPacket.moving_speed);
	
	return true;
}
