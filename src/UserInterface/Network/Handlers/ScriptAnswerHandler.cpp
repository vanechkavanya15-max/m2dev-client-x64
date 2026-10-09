#include "StdAfx.h"
/**
 * @file ScriptAnswerHandler.cpp
 * @brief Implementation of the script answer network handler.
 */

#include "ScriptAnswerHandler.h"
#include "../../PythonNetworkStream.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"

namespace Network::Handlers
{
    /**
     * @brief Sends a script answer packet over the provided network stream.
     * 
     * @param networkStream Pointer to the active network stream.
     * @param answer The zero-based index of the chosen answer.
     * @return true if the packet was successfully queued for sending.
     * @return false if the network stream is null or sending failed.
     */
    bool ScriptAnswerHandler::Send(CPythonNetworkStream* networkStream, uint8_t answer)
    {
        if (!networkStream)
        {
            return false;
        }

        Beavium::TPacketCGScriptAnswerBeavium packet{};
        packet.header = Beavium::CG::SCRIPT_ANSWER;
        packet.answer = answer;

        if (!networkStream->Send(sizeof(packet), &packet))
        {
            return false;
        }

        return true;
    }
}
