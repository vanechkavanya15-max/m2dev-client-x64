#include "../../StdAfx.h"
#include "StunHandler.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonCharacterManager.h"
#include "../../InstanceBase.h"
#include "../../Packet.h"

/**
 * @brief Handles the reception of the Stun packet from the network stream.
 * @param networkStream Pointer to the network stream object.
 * @return bool True if successful, false otherwise.
 */
bool StunHandler::Handle(CPythonNetworkStream* networkStream)
{
TPacketGCStun stunPacket;

    if (!networkStream->Recv(sizeof(stunPacket), &stunPacket))
    {
        // Use std::string_view for texts as per C++20 requirement
        constexpr std::string_view errorMsg = "CPythonNetworkStream::RecvStunPacket Error";
        Tracen(errorMsg.data());
        return false;
    }

    CPythonCharacterManager& characterManager = CPythonCharacterManager::Instance();
    CInstanceBase* targetInstance = characterManager.GetInstancePtr(stunPacket.vid);

    if (targetInstance)
    {
        if (characterManager.GetMainInstancePtr() == targetInstance)
        {
            targetInstance->Die();
        }
        else
        {
            targetInstance->Stun();
        }
    }

    return true;
}
